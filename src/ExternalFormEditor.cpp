#include "FormEditor.h"
#include "CodeGen.h"
#include <cstring>

namespace {
enum { Add=1, SetInt=2, SetStr=3, Begin=4, End=5, Select=6, Delete=7 };
struct AddPacket { char name[65]; int id, x, y, w, h; };
struct IntPacket { int command; char name[65]; int id, value; };
struct StringPacket { int command; char name[65]; int id; char value[65]; };
struct IdPacket { int command, id; };
bool wouldBlock(GError *error) { return g_error_matches(error, G_IO_ERROR, G_IO_ERROR_WOULD_BLOCK); }
bool integerProperty(DataType type) {
    return type == data_int || type == data_combo || type == data_comboEx;
}
}

void FE_ExternalTcp::start(SDK_Editor *editor) {
    stop(); FormEditor::start(editor);
    GError *error = nullptr;
    sock = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, &error);
    GInetAddress *loopback = g_inet_address_new_loopback(G_SOCKET_FAMILY_IPV4);
    GSocketAddress *address = g_inet_socket_address_new(loopback, requestedPort);
    g_object_unref(loopback);
#ifdef G_OS_WIN32
    const bool reuse = false; // Windows SO_REUSEADDR can permit another listener.
#else
    const bool reuse = true; // Permit reconnect while the old peer is in TIME_WAIT.
#endif
    bool ready = sock && g_socket_bind(sock, address, reuse, &error) && g_socket_listen(sock, &error);
    g_object_unref(address);
    if(ready) {
        auto local = g_socket_get_local_address(sock, &error);
        if(local) {
            boundPort = g_inet_socket_address_get_port(G_INET_SOCKET_ADDRESS(local));
            g_object_unref(local);
        } else ready = false;
    }
    if(ready) {
#ifdef G_OS_WIN32
        const auto executable = editor->sdk->pack->pathMake() + "formedit.exe";
#else
        const auto executable = editor->sdk->pack->pathMake() + "formedit";
#endif
        auto launcher = g_subprocess_launcher_new(G_SUBPROCESS_FLAGS_NONE);
        g_subprocess_launcher_set_cwd(launcher, editor->sdk->pack->pathMake().c_str());
        helper = g_subprocess_launcher_spawn(launcher, &error, executable.c_str(), nullptr);
        g_object_unref(launcher);
        ready = helper != nullptr;
    }
    if(!ready) {
        const std::string message = std::string("!Cannot start external form editor: ") + (error ? error->message : "unknown error");
        cgt_on_debug.run(message.c_str()); g_clear_error(&error); stop(); return;
    }
    g_socket_set_blocking(sock, false);
    editor->csdk->on_remove_element += this;
    editor->csdk->selMan->on_selection_change += this;
    editor->csdk->on_add_element += this;
    subscribed = true;
    deadline = g_get_monotonic_time() + 30 * G_TIME_SPAN_SECOND;
    timer = g_timeout_add(10, [](gpointer data) -> gboolean {
        auto self = static_cast<FE_ExternalTcp*>(data);
        if(self->poll()) return G_SOURCE_CONTINUE;
        self->timer = 0;
        // The callback may destroy the editor, so copy it before stopping and
        // never dereference self after invoking it.
        auto ended = self->on_end_edit;
        self->stop(); ended();
        return G_SOURCE_REMOVE;
    }, this);
}

void FE_ExternalTcp::stop() {
    if(timer) { g_source_remove(timer); timer = 0; }
    if(subscribed) {
        sde->csdk->on_remove_element -= this;
        sde->csdk->selMan->on_selection_change -= this;
        sde->csdk->on_add_element -= this;
        for(auto element : sde->csdk->elements) element->on_property_change -= this;
        subscribed = false;
    }
    for(GSocket **socket : {&client, &sock}) {
        if(*socket) { g_socket_close(*socket, nullptr); g_object_unref(*socket); *socket = nullptr; }
    }
    if(helper) { g_subprocess_force_exit(helper); g_object_unref(helper); helper = nullptr; }
    incoming.clear(); outgoing.clear(); on_end_edit.clear();
    boundPort = 0;
}

bool FE_ExternalTcp::sendBytes(const void *data, size_t size) {
    if(!client) return false;
    if(outgoing.size() + size > 4*1024*1024) { deadline = 0; return false; }
    const auto bytes = static_cast<const char*>(data);
    outgoing.insert(outgoing.end(), bytes, bytes + size); return true;
}
void FE_ExternalTcp::pk_send(int command, void *data, int size) {
    sendBytes(&command, sizeof(command));
    if(size > 0) sendBytes(data, static_cast<size_t>(size));
}
void FE_ExternalTcp::pk_add_element(Element *element) {
    if(!client || !element || !element->tpl) return;
    AddPacket packet{};
    g_strlcpy(packet.name, element->tpl->name.c_str(), sizeof(packet.name)); packet.id = element->id;
    auto value = [&](const char *name) { auto property = element->props.getByName(name); return property ? property->readInt() : 0; };
    packet.x = value("Left"); packet.y = value("Top"); packet.w = value("Width"); packet.h = value("Height");
    pk_send(Add, &packet, sizeof(packet));
    for(auto property : element->props) pk_send_prop(property);
    element->on_property_change -= this; element->on_property_change += this;
}
void FE_ExternalTcp::pk_send_prop(ElementProperty *property) {
    if(!client || !property || !property->parent) return;
    if(property->type == data_list || property->type == data_str) {
        StringPacket packet{}; packet.command = SetStr; packet.id = property->parent->id;
        g_strlcpy(packet.name, property->name.c_str(), sizeof(packet.name));
        g_strlcpy(packet.value, property->readStr().c_str(), sizeof(packet.value));
        sendBytes(&packet, sizeof(packet));
    } else if(integerProperty(property->type)) {
        IntPacket packet{}; packet.command = SetInt; packet.id = property->parent->id; packet.value = property->readInt();
        g_strlcpy(packet.name, property->name.c_str(), sizeof(packet.name)); sendBytes(&packet, sizeof(packet));
    }
}
void FE_ExternalTcp::pk_send_remove(Element *element) {
    element->on_property_change -= this;
    IdPacket packet{Delete, element->id}; sendBytes(&packet, sizeof(packet));
}
void FE_ExternalTcp::pk_send_select(SelectManager *selection) {
    IdPacket packet{Select, 0}; sendBytes(&packet, sizeof(packet));
    for(auto element : selection->elements) if(element->tpl->conf->isWinElement()) {
        packet.id = element->id; sendBytes(&packet, sizeof(packet));
    }
}
void FE_ExternalTcp::callback(void *owner, CallbackType type, const void *data) {
    switch(type) {
    case CBT_ELEMENT_PROP_CHANGE: pk_send_prop((ElementProperty*)data); break;
    case CBT_SDK_REMOVE_ELEMENT: pk_send_remove((Element*)data); break;
    case CBT_SDK_CHANGE_SELECT: pk_send_select((SelectManager*)owner); break;
    case CBT_SDK_ADD_ELEMENT:
        if(((Element*)data)->tpl->conf->isWinElement()) pk_add_element((Element*)data);
        break;
    default: break;
    }
}
bool FE_ExternalTcp::applyPacket(int command, const char *bytes) {
    if(command == SetInt) {
        IntPacket packet; memcpy(&packet, bytes, sizeof(packet));
        if(!memchr(packet.name, 0, sizeof(packet.name)) || !g_utf8_validate(packet.name,-1,nullptr)) return false;
        auto element = sde->csdk->getElementById(packet.id);
        if(!element || (element->flag & ELEMENT_FLG_IS_FREEZE)) return true;
        auto property = element->props.getByName(packet.name);
        if(!property || !integerProperty(property->type)) return false;
        element->on_property_change -= this; property->setValue(&packet.value); element->on_property_change += this;
        sde->changeScheme(); return true;
    }
    IdPacket packet; memcpy(&packet, bytes, sizeof(packet));
    auto element = sde->csdk->getElementById(packet.id);
    if(command == Select) {
        if(!packet.id) sde->csdk->selMan->clear();
        else if(element) sde->csdk->selMan->add(element);
        return true;
    }
    if(command == Delete) {
        if(element && !element->isParent() && !(element->flag & ELEMENT_FLG_IS_FREEZE)) {
            // SelectManager::add expands groups; delete only the requested ID.
            std::vector<int> selected;
            for(auto item : sde->csdk->selMan->elements) if(item != element) selected.push_back(item->id);
            sde->csdk->selMan->clear(); sde->csdk->remove(element);
            for(int id : selected) if(auto item = sde->csdk->getElementById(id)) sde->csdk->selMan->add(item);
            sde->changeScheme();
        }
        return true;
    }
    return false;
}
bool FE_ExternalTcp::poll() {
    GError *error = nullptr;
    if(!deadline) return false;
    if(!client) {
        if(g_get_monotonic_time() > deadline) { cgt_on_debug.run("!External form editor connection timed out"); return false; }
        client = g_socket_accept(sock, nullptr, &error);
        if(!client) { bool retry = wouldBlock(error); g_clear_error(&error); return retry; }
        g_socket_set_blocking(client, false);
        pk_send(Begin, nullptr, 0);
        for(auto element : sde->csdk->elements) if(element->isParent()) { pk_add_element(element); break; }
        for(auto element : sde->csdk->elements) if(!element->isParent() && element->tpl->conf->isWinElement()) pk_add_element(element);
        pk_send(End, nullptr, 0);
    }
    if(!outgoing.empty()) {
        const auto sent = g_socket_send(client, outgoing.data(), outgoing.size(), nullptr, &error);
        if(sent < 0) { bool retry = wouldBlock(error); g_clear_error(&error); if(!retry) return false; }
        else if(sent == 0) return false;
        else outgoing.erase(outgoing.begin(), outgoing.begin()+sent);
    }
    char buffer[4096];
    const auto received = g_socket_receive(client, buffer, sizeof(buffer), nullptr, &error);
    if(received < 0) { bool retry = wouldBlock(error); g_clear_error(&error); return retry; }
    if(received == 0) return false;
    incoming.insert(incoming.end(), buffer, buffer+received);
    while(incoming.size() >= sizeof(int)) {
        int command; memcpy(&command, incoming.data(), sizeof(command));
        const size_t size = command == SetInt ? sizeof(IntPacket) : (command == Select || command == Delete) ? sizeof(IdPacket) : 0;
        if(!size) { cgt_on_debug.run("!Invalid external form editor command"); return false; }
        if(incoming.size() < size) break;
        if(!applyPacket(command, incoming.data())) return false;
        incoming.erase(incoming.begin(), incoming.begin()+size);
    }
    return true;
}
