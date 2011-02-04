<?php
    include_once "vz_cmd.php";

    $a = array();

    vz_cmd::RESET_DIRECTOR($a, 'd_test');
    vz_cmd::SET_TEXT($a, 't_name', 'Name');

    $r = vz_cmd::send("dev-2", $a);

    print "r=$r\n";
?>
