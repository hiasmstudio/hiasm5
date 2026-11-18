function chmod_to_str($file) {
    $perms = fileperms($file);
    
    if (($perms & 0xC000) == 0xC000) {
        // Сокет
        $info = 's';
    } elseif (($perms & 0xA000) == 0xA000) {
        // Символическая ссылка
        $info = 'l';
    } elseif (($perms & 0x8000) == 0x8000) {
        // Обычный
        $info = '-';
    } elseif (($perms & 0x6000) == 0x6000) {
        // Специальный блок
        $info = 'b';
    } elseif (($perms & 0x4000) == 0x4000) {
        // Директория
        $info = 'd';
    } elseif (($perms & 0x2000) == 0x2000) {
        // Специальный символ
        $info = 'c';
    } elseif (($perms & 0x1000) == 0x1000) {
        // Поток FIFO
        $info = 'p';
    } else {
        // Неизвестный
        $info = 'u';
    }
    
    // Владелец
    $info .= (($perms & 0x0100) ? 'r' : '-');
    $info .= (($perms & 0x0080) ? 'w' : '-');
    $info .= (($perms & 0x0040) ?
                (($perms & 0x0800) ? 's' : 'x' ) :
                (($perms & 0x0800) ? 'S' : '-'));
    
    // Группа
    $info .= (($perms & 0x0020) ? 'r' : '-');
    $info .= (($perms & 0x0010) ? 'w' : '-');
    $info .= (($perms & 0x0008) ?
                (($perms & 0x0400) ? 's' : 'x' ) :
                (($perms & 0x0400) ? 'S' : '-'));
    
    // Мир
    $info .= (($perms & 0x0004) ? 'r' : '-');
    $info .= (($perms & 0x0002) ? 'w' : '-');
    $info .= (($perms & 0x0001) ?
                (($perms & 0x0200) ? 't' : 'x' ) :
                (($perms & 0x0200) ? 'T' : '-'));
    return $info;
}

function str_to_chmod($permissions) {
    $mode = 0;
    
    if ($permissions[1] == 'r') $mode += 0400;
    if ($permissions[2] == 'w') $mode += 0200;
    if ($permissions[3] == 'x') $mode += 0100;
    else if ($permissions[3] == 's') $mode += 04100;
    else if ($permissions[3] == 'S') $mode += 04000;
    
    if ($permissions[4] == 'r') $mode += 040;
    if ($permissions[5] == 'w') $mode += 020;
    if ($permissions[6] == 'x') $mode += 010;
    else if ($permissions[6] == 's') $mode += 02010;
    else if ($permissions[6] == 'S') $mode += 02000;
    
    if ($permissions[7] == 'r') $mode += 04;
    if ($permissions[8] == 'w') $mode += 02;
    if ($permissions[9] == 'x') $mode += 01;
    else if ($permissions[9] == 't') $mode += 01001;
    else if ($permissions[9] == 'T') $mode += 01000;
    return $mode;
}