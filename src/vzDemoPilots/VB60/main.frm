VERSION 5.00
Begin VB.Form MainForm 
   Caption         =   "vzDemoPilotVB60 (demo pilot by Visual Basic 6.0)"
   ClientHeight    =   3345
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   9945
   LinkTopic       =   "Form1"
   ScaleHeight     =   3345
   ScaleWidth      =   9945
   StartUpPosition =   3  'Windows Default
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
Private Sub demo1_director_cont_Click()
    Dim argv(0 To 2) As vzCmdSendPart_desc

    ' Setup command
    argv(0).cmd = VZ_CMD_CONTINUE_DIRECTOR
    argv(1).cmd = "main"
    argv(2).cmd = "0"

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
    argv(1).cmd = "text_2"
    argv(2).cmd = "s_text"
    argv(3).cmd = demo1_t_1.Text
    argv(4).cmd = VZ_CMD_SET
    argv(5).cmd = "text_1"
    argv(6).cmd = "s_text"
    argv(7).cmd = demo1_t_2.Text

    send_vz_data argv
End Sub
