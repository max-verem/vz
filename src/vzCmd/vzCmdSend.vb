Module vzCmdSend

    Public Declare Function vzCmdSendVB Lib "vzCmdSend" Alias "_vzCmdSendVB2008@12" _
        (ByVal host As String, _
        ByVal count As Integer, _
        <Runtime.InteropServices.MarshalAs(Runtime.InteropServices.UnmanagedType.SafeArray, _
            SafeArraySubType:=Runtime.InteropServices.VarEnum.VT_BSTR)> _
        ByRef data() As String) As Integer

    ' text constants for command naming
    Public Const VZ_CMD_LOAD_SCENE As String = "LOAD_SCENE"
    Public Const VZ_CMD_START_DIRECTOR As String = "START_DIRECTOR"
    Public Const VZ_CMD_RESET_DIRECTOR As String = "RESET_DIRECTOR"
    Public Const VZ_CMD_CONTINUE_DIRECTOR As String = "CONTINUE_DIRECTOR"
    Public Const VZ_CMD_STOP_DIRECTOR As String = "STOP_DIRECTOR"
    Public Const VZ_CMD_SET As String = "SET"
    Public Const VZ_CMD_PING As String = "PING"
    Public Const VZ_CMD_CONTAINER_VISIBLE As String = "CONTAINER_VISIBLE"
    Public Const VZ_CMD_SCREENSHOT As String = "SCREENSHOT"

End Module
