#include "FormEditor.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include <glib/gstdio.h>
#include <cstring>
#include <iostream>

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)
static void drain(int ms=70) {
    const auto end=g_get_monotonic_time()+ms*1000;
    while(g_get_monotonic_time()<end) { while(g_main_context_iteration(nullptr,false)) {} g_usleep(1000); }
}
static void ended(void *value) { *static_cast<bool*>(value)=true; }
int main(int argc,char **argv) {
    if(argc!=3) return 2;
#ifdef HIASM_GTK4
    auto app=Gtk::Application::create("com.hiasm.external-form-test",Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc,argv);
#endif
    initDirs(); dataDir=ustring(argv[1])+G_DIR_SEPARATOR_S;
    databaseFile=dataDir+INT_PATH+DATABASE_FILE; mdb.open(); SettingsManager::init();
    compilerSet=new CompilerCollection(); packSet=new PackCollection();
    gchar *temporary=g_dir_make_tmp("hiasm-external-form-XXXXXX",nullptr); CHECK(temporary);
    const ustring root(temporary); g_free(temporary);
    CHECK(g_mkdir_with_parents((root+"/make").c_str(),0700)==0);
#ifdef G_OS_WIN32
    const auto executable=root+"/make/formedit.exe";
#else
    const auto executable=root+"/make/formedit";
#endif
    GFile *from=g_file_new_for_path(argv[2]),*to=g_file_new_for_path(executable.c_str());
    CHECK(g_file_copy(from,to,G_FILE_COPY_NONE,nullptr,nullptr,nullptr,nullptr));
    g_object_unref(from); g_object_unref(to);
#ifndef G_OS_WIN32
    CHECK(g_chmod(executable.c_str(),0700)==0);
#endif
    Pack fixture("external-fixture","test helper",root,root);
    SDK_Editor editor; editor.sdk->pack=packSet->getPackByName("_base");
    auto a=editor.sdk->add("hcTplButton",0,0),b=editor.sdk->add("hcTplButton",100,0);
    CHECK(a&&b); const int aid=a->id,bid=b->id;
    editor.sdk->pack=&fixture;
    // The production helper uses 4588. Tests reserve an ephemeral socket so
    // GTK3/GTK4 build trees and a running IDE cannot collide.
    FE_ExternalTcp external(0);
    for(int attempt=0;attempt<3;++attempt) {
        bool closed=false;
        external.start(&editor); external.on_end_edit=ObjectEvent(&closed,ended);
        CHECK(external.isActive());
        CHECK(external.port()!=0);
        auto address=g_inet_socket_address_new_from_string("127.0.0.1",external.port());
        GSocket *peer=g_socket_new(G_SOCKET_FAMILY_IPV4,G_SOCKET_TYPE_STREAM,G_SOCKET_PROTOCOL_TCP,nullptr);
        CHECK(g_socket_connect(peer,address,nullptr,nullptr)); g_object_unref(address);
        g_socket_set_timeout(peer,2); drain(); CHECK(!closed);
        char initial[16384]; CHECK(g_socket_receive(peer,initial,sizeof(initial),nullptr,nullptr)>0);
        if(attempt==0) {
            struct { int command; char name[65]; int id,value; } packet{};
            packet.command=2; g_strlcpy(packet.name,"Left",sizeof(packet.name)); packet.id=aid; packet.value=123;
            const char *bytes=reinterpret_cast<const char*>(&packet);
            CHECK(g_socket_send(peer,bytes,2,nullptr,nullptr)==2); drain();
            CHECK(a->props.getByName("Left")->readInt()!=123);
            CHECK(g_socket_send(peer,bytes+2,sizeof(packet)-2,nullptr,nullptr)==sizeof(packet)-2); drain();
            CHECK(a->props.getByName("Left")->readInt()==123);
            // A non-integer property must not be overwritten through the int ABI.
            g_strlcpy(packet.name,"Caption",sizeof(packet.name));
            CHECK(g_socket_send(peer,reinterpret_cast<char*>(&packet),sizeof(packet),nullptr,nullptr)==sizeof(packet));
            drain(); CHECK(closed);
        } else if(attempt==1) {
            struct { int command,id; } selected{6,bid},remove{7,aid};
            CHECK(g_socket_send(peer,reinterpret_cast<char*>(&selected),sizeof(selected),nullptr,nullptr)==sizeof(selected)); drain();
            CHECK(editor.csdk->selMan->getFirst()==b);
            CHECK(g_socket_send(peer,reinterpret_cast<char*>(&remove),sizeof(remove),nullptr,nullptr)==sizeof(remove)); drain();
            CHECK(!editor.sdk->getElementById(aid)&&editor.sdk->getElementById(bid));
            g_socket_close(peer,nullptr); drain(); CHECK(closed);
        } else {
            // Closing while connected must remove the timeout and subscriptions.
            external.stop(); external.stop(); drain(); CHECK(!closed);
        }
        g_socket_close(peer,nullptr); g_object_unref(peer); external.stop();
    }
    editor.sdk->pack=packSet->getPackByName("_base");
    CHECK(g_unlink(executable.c_str())==0); g_rmdir((root+"/make").c_str()); g_rmdir(root.c_str());
    std::cout<<"OK external form editor: partial packets, type validation, target deletion, reconnect and teardown"<<std::endl;
}
