VERSION 5.00
Begin VB.Form MainForm 
   Caption         =   "vzDemoPilotVB60 (demo pilot by Visual Basic 6.0)"
   ClientHeight    =   9075
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9945
   LinkTopic       =   "Form1"
   ScaleHeight     =   9075
   ScaleWidth      =   9945
   StartUpPosition =   3  'Windows Default
   Begin VB.Frame Frame5 
      Caption         =   "Other"
      Height          =   1455
      Left            =   0
      TabIndex        =   45
      Top             =   7560
      Width           =   9855
      Begin VB.CommandButton b_load_other_2 
         Caption         =   "image_test_ft.xml"
         Height          =   375
         Left            =   120
         TabIndex        =   48
         Top             =   840
         Width           =   1815
      End
      Begin VB.CommandButton b_load_other_1 
         Caption         =   "cube.xml"
         Height          =   375
         Left            =   120
         TabIndex        =   46
         Top             =   240
         Width           =   1815
      End
      Begin VB.Label Label12 
         Caption         =   "Image free transform"
         Height          =   255
         Left            =   2040
         TabIndex        =   49
         Top             =   960
         Width           =   1935
      End
      Begin VB.Label Label11 
         Caption         =   "Trajectory demo"
         Height          =   255
         Left            =   2040
         TabIndex        =   47
         Top             =   360
         Width           =   2415
      End
   End
   Begin VB.Frame Frame4 
      Caption         =   "demo5.xml"
      Height          =   1935
      Left            =   0
      TabIndex        =   27
      Top             =   5520
      Width           =   9855
      Begin VB.CommandButton b_demo5_f1 
         Caption         =   "MM:SS.DDD"
         Height          =   375
         Left            =   4920
         TabIndex        =   44
         Top             =   1440
         Width           =   1455
      End
      Begin VB.CommandButton b_demo5_f2 
         Caption         =   "m:SS"
         Height          =   375
         Left            =   6480
         TabIndex        =   43
         Top             =   1440
         Width           =   1575
      End
      Begin VB.CommandButton b_demo5_f3 
         Caption         =   "HH:MM:SS:FF"
         Height          =   375
         Left            =   8160
         TabIndex        =   42
         Top             =   1440
         Width           =   1575
      End
      Begin VB.TextBox t_demo5_start_value 
         Height          =   375
         Left            =   1320
         TabIndex        =   38
         Text            =   "10000"
         Top             =   960
         Width           =   1215
      End
      Begin VB.CommandButton b_demo5_set_start 
         Caption         =   "SET"
         Height          =   375
         Left            =   2640
         TabIndex        =   37
         Top             =   960
         Width           =   615
      End
      Begin VB.TextBox t_demo5_stop_value 
         Height          =   375
         Left            =   1320
         TabIndex        =   35
         Text            =   "20000"
         Top             =   1440
         Width           =   1215
      End
      Begin VB.CommandButton b_demo5_set_stop 
         Caption         =   "SET"
         Height          =   375
         Left            =   2640
         TabIndex        =   34
         Top             =   1440
         Width           =   615
      End
      Begin VB.CommandButton b_demo5_start 
         Caption         =   "Start"
         Height          =   375
         Left            =   5520
         TabIndex        =   33
         Top             =   240
         Width           =   1095
      End
      Begin VB.CommandButton b_demo5_stop 
         Caption         =   "Stop"
         Height          =   375
         Left            =   6720
         TabIndex        =   32
         Top             =   240
         Width           =   975
      End
      Begin VB.CommandButton b_demo5_cont 
         Caption         =   "Continue"
         Height          =   375
         Left            =   7800
         TabIndex        =   31
         Top             =   240
         Width           =   855
      End
      Begin VB.CommandButton b_demo5_reset 
         Caption         =   "Reset"
         Height          =   375
         Left            =   8760
         TabIndex        =   30
         Top             =   240
         Width           =   975
      End
      Begin VB.CommandButton b_demo5_load_scene 
         Caption         =   "Load Scene"
         Height          =   375
         Left            =   120
         TabIndex        =   28
         Top             =   240
         Width           =   1215
      End
      Begin VB.Label Label10 
         Caption         =   "Timer Format"
         Height          =   255
         Left            =   6480
         TabIndex        =   41
         Top             =   1080
         Width           =   1215
      End
      Begin VB.Label Label9 
         Caption         =   "Timer's range values (miliseconds)"
         Height          =   255
         Left            =   600
         TabIndex        =   40
         Top             =   720
         Width           =   2895
      End
      Begin VB.Label Label8 
         Alignment       =   1  'Right Justify
         Caption         =   "Start Value:"
         Height          =   255
         Left            =   120
         TabIndex        =   39
         Top             =   1080
         Width           =   1095
      End
      Begin VB.Label Label7 
         Alignment       =   1  'Right Justify
         Caption         =   "Stop Value:"
         Height          =   255
         Left            =   120
         TabIndex        =   36
         Top             =   1560
         Width           =   1095
      End
      Begin VB.Label Label6 
         Alignment       =   1  'Right Justify
         Caption         =   "Runtime Control:"
         Height          =   255
         Left            =   4080
         TabIndex        =   29
         Top             =   360
         Width           =   1335
      End
   End
   Begin VB.CommandButton b_demo3_speed30 
      Caption         =   "3.0"
      Height          =   375
      Left            =   5400
      TabIndex        =   26
      Top             =   4800
      Width           =   495
   End
   Begin VB.CommandButton b_demo3_speed15 
      Caption         =   "1.5"
      Height          =   375
      Left            =   3240
      TabIndex        =   23
      Top             =   4800
      Width           =   495
   End
   Begin VB.Frame Frame3 
      Caption         =   "demo3.xml"
      Height          =   2055
      Left            =   0
      TabIndex        =   13
      Top             =   3360
      Width           =   9855
      Begin VB.CommandButton b_demo3_speed25 
         Caption         =   "2.5"
         Height          =   375
         Left            =   4680
         TabIndex        =   25
         Top             =   1440
         Width           =   495
      End
      Begin VB.CommandButton b_demo3_speed20 
         Caption         =   "2.0"
         Height          =   375
         Left            =   3960
         TabIndex        =   24
         Top             =   1440
         Width           =   495
      End
      Begin VB.CommandButton b_demo3_speed10 
         Caption         =   "1.0"
         Height          =   375
         Left            =   2520
         TabIndex        =   22
         Top             =   1440
         Width           =   495
      End
      Begin VB.CommandButton b_demo3_reset 
         Caption         =   "Reset"
         Height          =   375
         Left            =   8520
         TabIndex        =   20
         Top             =   240
         Width           =   1215
      End
      Begin VB.CommandButton b_demo3_loop_off 
         Caption         =   "Loop OFF"
         Height          =   375
         Left            =   7320
         TabIndex        =   19
         Top             =   240
         Width           =   1095
      End
      Begin VB.CommandButton b_demo3_loop_on 
         Caption         =   "Loop ON"
         Height          =   375
         Left            =   6120
         TabIndex        =   18
         Top             =   240
         Width           =   1095
      End
      Begin VB.TextBox t_demo3_text 
         Height          =   375
         Left            =   120
         TabIndex        =   16
         Text            =   "NEWS: VZ has a simple VB6.0 integration"
         Top             =   960
         Width           =   9615
      End
      Begin VB.CommandButton b_demo3_load_scene 
         Caption         =   "Load Scene"
         Height          =   375
         Left            =   120
         TabIndex        =   15
         Top             =   240
         Width           =   1215
      End
      Begin VB.CommandButton b_demo3_append 
         Caption         =   "Append Texts"
         Height          =   375
         Left            =   1440
         TabIndex        =   14
         Top             =   240
         Width           =   1215
      End
      Begin VB.Label Label5 
         Caption         =   "Speed control:"
         Height          =   255
         Left            =   240
         TabIndex        =   21
         Top             =   1560
         Width           =   1455
      End
      Begin VB.Label Label4 
         Caption         =   "Craw text block"
         Height          =   255
         Left            =   120
         TabIndex        =   17
         Top             =   720
         Width           =   1095
      End
   End
   Begin VB.Frame Frame2 
      Caption         =   "demo1.xml"
      Height          =   2295
      Left            =   0
      TabIndex        =   3
      Top             =   960
      Width           =   9855
      Begin VB.TextBox demo1_t_2 
         Height          =   375
         Left            =   120
         TabIndex        =   10
         Text            =   "Vasilij Pupkin"
         Top             =   1680
         Width           =   9615
      End
      Begin VB.TextBox demo1_t_1 
         Height          =   375
         Left            =   120
         TabIndex        =   9
         Text            =   "Senior Deputy Officer"
         Top             =   960
         Width           =   9615
      End
      Begin VB.CommandButton demo1_director_reset 
         Caption         =   "Reset Animation"
         Height          =   375
         Left            =   8280
         TabIndex        =   8
         Top             =   240
         Width           =   1455
      End
      Begin VB.CommandButton demo1_director_cont 
         Caption         =   "Continue Animation"
         Height          =   375
         Left            =   6480
         TabIndex        =   7
         Top             =   240
         Width           =   1695
      End
      Begin VB.CommandButton demo1_director_start 
         Caption         =   "Start Animation"
         Height          =   375
         Left            =   4920
         TabIndex        =   6
         Top             =   240
         Width           =   1455
      End
      Begin VB.CommandButton demo1_load_texts 
         Caption         =   "Load Texts"
         Height          =   375
         Left            =   1440
         TabIndex        =   5
         Top             =   240
         Width           =   1215
      End
      Begin VB.CommandButton demo1_load_scene 
         Caption         =   "Load Scene"
         Height          =   375
         Left            =   120
         TabIndex        =   4
         Top             =   240
         Width           =   1215
      End
      Begin VB.Label Label3 
         Caption         =   "Line #2"
         Height          =   255
         Left            =   120
         TabIndex        =   12
         Top             =   1440
         Width           =   1215
      End
      Begin VB.Label Label2 
         Caption         =   "Line #1"
         Height          =   255
         Left            =   120
         TabIndex        =   11
         Top             =   720
         Width           =   1095
      End
   End
   Begin VB.Frame Frame1 
      Caption         =   "Host info"
      Height          =   735
      Left            =   0
      TabIndex        =   0
      Top             =   120
      Width           =   9855
      Begin VB.TextBox t_host 
         Height          =   405
         Left            =   3120
         TabIndex        =   1
         Text            =   "localhost"
         Top             =   165
         Width           =   3135
      End
      Begin VB.Label Label1 
         Alignment       =   1  'Right Justify
         Caption         =   "VZ hostname or IP address:"
         Height          =   255
         Left            =   240
         TabIndex        =   2
         Top             =   240
         Width           =   2775
      End
   End
End
Attribute VB_Name = "MainForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Private Sub send_vz_data(argv() As vzCmdSendPart_desc)
    Dim r As Long
    Dim count As Long
    Dim start As Long
    
    ' calc count of elements
    start = LBound(argv)
    count = UBound(argv) - start + 1
    
    ' Send command
    r = vzCmdSendVB(t_host.Text, count, VarPtr(argv(start)))

End Sub

Private Sub b_demo3_append_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "text_crawl"
    argv(2).cmd = "s_trig_append"
    argv(3).cmd = t_demo3_text.Text

    send_vz_data argv
End Sub

Private Sub b_demo3_load_scene_Click()
    Dim argv(0 To 1) As vzCmdSendPart_desc

    argv(0).cmd = VZ_CMD_LOAD_SCENE
    argv(1).cmd = "./projects/demo3.xml"

    send_vz_data argv
End Sub

Private Sub b_demo3_loop_off_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "text_crawl"
    argv(2).cmd = "l_loop"
    argv(3).cmd = "0"

    send_vz_data argv
End Sub

Private Sub b_demo3_loop_on_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "text_crawl"
    argv(2).cmd = "l_loop"
    argv(3).cmd = "1"

    send_vz_data argv
End Sub

Private Sub b_demo3_reset_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "text_crawl"
    argv(2).cmd = "l_reset"
    argv(3).cmd = "1"

    send_vz_data argv
End Sub

Private Sub b_demo3_speed10_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "text_crawl"
    argv(2).cmd = "f_speed"
    argv(3).cmd = "1.0"

    send_vz_data argv
End Sub

Private Sub b_demo3_speed15_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "text_crawl"
    argv(2).cmd = "f_speed"
    argv(3).cmd = "1.5"

    send_vz_data argv
End Sub

Private Sub b_demo3_speed20_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "text_crawl"
    argv(2).cmd = "f_speed"
    argv(3).cmd = "2.0"

    send_vz_data argv
End Sub

Private Sub b_demo3_speed25_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "text_crawl"
    argv(2).cmd = "f_speed"
    argv(3).cmd = "2.5"

    send_vz_data argv
End Sub

Private Sub b_demo3_speed30_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "text_crawl"
    argv(2).cmd = "f_speed"
    argv(3).cmd = "4.0"

    send_vz_data argv
End Sub

Private Sub b_demo5_cont_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "timer1"
    argv(2).cmd = "l_trig_cont"
    argv(3).cmd = "1"

    send_vz_data argv
End Sub

Private Sub b_demo5_f1_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "timer1"
    argv(2).cmd = "s_format"
    argv(3).cmd = "MM:SS.DDD"

    send_vz_data argv
End Sub

Private Sub b_demo5_f2_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "timer1"
    argv(2).cmd = "s_format"
    argv(3).cmd = "m:SS"

    send_vz_data argv
End Sub

Private Sub b_demo5_f3_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "timer1"
    argv(2).cmd = "s_format"
    argv(3).cmd = "HH:MM:SS:FF"
    send_vz_data argv
End Sub

Private Sub b_demo5_load_scene_Click()
    Dim argv(0 To 1) As vzCmdSendPart_desc

    argv(0).cmd = VZ_CMD_LOAD_SCENE
    argv(1).cmd = "./projects/demo5.xml"

    send_vz_data argv
End Sub

Private Sub b_demo5_reset_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "timer1"
    argv(2).cmd = "l_trig_reset"
    argv(3).cmd = "1"

    send_vz_data argv
End Sub

Private Sub b_demo5_set_start_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "timer1"
    argv(2).cmd = "l_start_value"
    argv(3).cmd = t_demo5_start_value.Text

    send_vz_data argv
End Sub

Private Sub b_demo5_set_stop_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "timer1"
    argv(2).cmd = "l_stop_value"
    argv(3).cmd = t_demo5_stop_value.Text

    send_vz_data argv
End Sub

Private Sub b_demo5_start_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "timer1"
    argv(2).cmd = "l_trig_start"
    argv(3).cmd = "1"

    send_vz_data argv
End Sub

Private Sub b_demo5_stop_Click()
    Dim argv(0 To 3) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "timer1"
    argv(2).cmd = "l_trig_stop"
    argv(3).cmd = "1"

    send_vz_data argv
End Sub

Private Sub b_load_other_1_Click()
    Dim argv(0 To 1) As vzCmdSendPart_desc

    argv(0).cmd = VZ_CMD_LOAD_SCENE
    argv(1).cmd = "./projects/cube.xml"

    send_vz_data argv
End Sub

Private Sub b_load_other_2_Click()
    Dim argv(0 To 1) As vzCmdSendPart_desc

    argv(0).cmd = VZ_CMD_LOAD_SCENE
    argv(1).cmd = "./projects/image_test_ft.xml"

    send_vz_data argv
End Sub

Private Sub demo1_director_cont_Click()
    Dim argv(0 To 1) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_CONTINUE_DIRECTOR
    argv(1).cmd = "main"

    send_vz_data argv
End Sub
Private Sub demo1_director_reset_Click()
    Dim argv(0 To 2) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_RESET_DIRECTOR
    argv(1).cmd = "main"
    argv(2).cmd = "0"

    send_vz_data argv
End Sub
Private Sub demo1_director_start_Click()
    Dim argv(0 To 2) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_START_DIRECTOR
    argv(1).cmd = "main"
    argv(2).cmd = "0"

    send_vz_data argv
End Sub
Private Sub demo1_load_scene_Click()
    Dim argv(0 To 1) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_LOAD_SCENE
    argv(1).cmd = "./projects/demo1.xml"

    send_vz_data argv
End Sub
Private Sub demo1_load_texts_Click()
    Dim argv(0 To 7) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_SET
    argv(1).cmd = "text_1"
    argv(2).cmd = "s_text"
    argv(3).cmd = demo1_t_1.Text
    argv(4).cmd = VZ_CMD_SET
    argv(5).cmd = "text_2"
    argv(6).cmd = "s_text"
    argv(7).cmd = demo1_t_2.Text

    send_vz_data argv
End Sub
