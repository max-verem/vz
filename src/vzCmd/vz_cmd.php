<?php

class vz_cmd
{
    public static function LOAD_SCENE(&$cmd_array, $filename)
    {
        array_push($cmd_array, array('LOAD_SCENE', $filename));
    }

    public static function START_DIRECTOR(&$cmd_array, $director, $pos = "0.0")
    {
        array_push($cmd_array, array('START_DIRECTOR', $director, $pos));
    }

    public static function RESET_DIRECTOR(&$cmd_array, $director, $pos = "0.0")
    {
        array_push($cmd_array, array('RESET_DIRECTOR', $director, $pos));
    }

    public static function CONTINUE_DIRECTOR(&$cmd_array, $director)
    {
        array_push($cmd_array, array('CONTINUE_DIRECTOR', $director));
    }

    public static function STOP_DIRECTOR(&$cmd_array, $director)
    {
        array_push($cmd_array, array('STOP_DIRECTOR', $director));
    }

    public static function SET(&$cmd_array, $container, $param, $value)
    {
        array_push($cmd_array, array('SET', $container, $param, $value));
    }

    public static function SET_TEXT(&$cmd_array, $container, $value)
    {
        self::SET($cmd_array, $container, 's_text', $value);
    }

    public static function SET_FILENAME(&$cmd_array, $container, $value)
    {
        self::SET($cmd_array, $container, 's_filename', $value);
    }

    public static function CONTAINER_VISIBLE(&$cmd_array, $container, $flag)
    {
        array_push($cmd_array, array('CONTAINER_VISIBLE', $container, $flag));
    }

    public static function LAYER_LOAD(&$cmd_array, $filename, $layer_number)
    {
        array_push($cmd_array, array('LAYER_LOAD', $filename, $layer_number));
    }

    public static function LAYER_UNLOAD(&$cmd_array, $layer_number)
    {
        array_push($cmd_array, array('LAYER_UNLOAD', $layer_number));
    }

    public static function send($host, &$cmd_array)
    {
        $a = array();

        foreach($cmd_array as $cmd)
            $a = array_merge($a, $cmd);

        return vz_send($host, $a);
    }
}

?>
