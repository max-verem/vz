Attribute VB_Name = "vzCmdSend"
' Based on
'   http://support.microsoft.com/kb/194609/EN-US/ example
'   http://forums.devx.com/archive/index.php/t-65745.html

'Struct to send
Type vzCmdSendPart_desc
    cmd As String
End Type

' function exported from DLL as
' VZCMDSEND_API long __stdcall vzCmdSendVB(char* host, long count, struct vzCmdSendPart_desc *data)

Public Declare Function vzCmdSendVB Lib "vzCmdSend" Alias "_vzCmdSendVB@12" _
    (ByVal host As String, _
    ByVal count As Long, ByVal data As Long) As Long

' text constants for command naming
Public Const VZ_CMD_LOAD_SCENE          As String = "LOAD_SCENE"
Public Const VZ_CMD_START_DIRECTOR      As String = "START_DIRECTOR"
Public Const VZ_CMD_RESET_DIRECTOR      As String = "RESET_DIRECTOR"
Public Const VZ_CMD_CONTINUE_DIRECTOR   As String = "CONTINUE_DIRECTOR"
Public Const VZ_CMD_STOP_DIRECTOR       As String = "STOP_DIRECTOR"
Public Const VZ_CMD_SET                 As String = "SET"
Public Const VZ_CMD_PING                As String = "PING"
Public Const VZ_CMD_CONTAINER_VISIBLE   As String = "CONTAINER_VISIBLE"
Public Const VZ_CMD_SCREENSHOT          As String = "SCREENSHOT"

