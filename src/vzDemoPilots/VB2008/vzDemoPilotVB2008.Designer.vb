<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class MainForm
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Me.TabControl1 = New System.Windows.Forms.TabControl
        Me.TabPage1 = New System.Windows.Forms.TabPage
        Me.b_demo1_director_reset = New System.Windows.Forms.Button
        Me.b_demo1_director_continue = New System.Windows.Forms.Button
        Me.b_demo1_director_start = New System.Windows.Forms.Button
        Me.b_demo1_load_text = New System.Windows.Forms.Button
        Me.t_demo1_t2 = New System.Windows.Forms.TextBox
        Me.Label3 = New System.Windows.Forms.Label
        Me.t_demo1_t1 = New System.Windows.Forms.TextBox
        Me.Label2 = New System.Windows.Forms.Label
        Me.b_demo1_load_scene = New System.Windows.Forms.Button
        Me.TabPage2 = New System.Windows.Forms.TabPage
        Me.b_demo3_speed30 = New System.Windows.Forms.Button
        Me.b_demo3_speed25 = New System.Windows.Forms.Button
        Me.b_demo3_speed20 = New System.Windows.Forms.Button
        Me.b_demo3_speed15 = New System.Windows.Forms.Button
        Me.b_demo3_speed10 = New System.Windows.Forms.Button
        Me.Label5 = New System.Windows.Forms.Label
        Me.b_demo3_reset = New System.Windows.Forms.Button
        Me.b_demo3_loop_off = New System.Windows.Forms.Button
        Me.b_demo3_loop_on = New System.Windows.Forms.Button
        Me.b_demo3_append = New System.Windows.Forms.Button
        Me.t_demo3_text = New System.Windows.Forms.TextBox
        Me.Label4 = New System.Windows.Forms.Label
        Me.b_demo3_load_scene = New System.Windows.Forms.Button
        Me.TabPage3 = New System.Windows.Forms.TabPage
        Me.b_demo5_set_stop = New System.Windows.Forms.Button
        Me.b_demo5_set_start = New System.Windows.Forms.Button
        Me.t_demo5_stop_value = New System.Windows.Forms.TextBox
        Me.t_demo5_start_value = New System.Windows.Forms.TextBox
        Me.Label10 = New System.Windows.Forms.Label
        Me.Label9 = New System.Windows.Forms.Label
        Me.Label8 = New System.Windows.Forms.Label
        Me.b_demo5_cont = New System.Windows.Forms.Button
        Me.b_demo5_reset = New System.Windows.Forms.Button
        Me.b_demo5_stop = New System.Windows.Forms.Button
        Me.b_demo5_start = New System.Windows.Forms.Button
        Me.Label7 = New System.Windows.Forms.Label
        Me.b_demo5_f2 = New System.Windows.Forms.Button
        Me.b_demo5_f3 = New System.Windows.Forms.Button
        Me.b_demo5_f1 = New System.Windows.Forms.Button
        Me.Label6 = New System.Windows.Forms.Label
        Me.b_demo5_load_scene = New System.Windows.Forms.Button
        Me.TabPage4 = New System.Windows.Forms.TabPage
        Me.Label12 = New System.Windows.Forms.Label
        Me.Label11 = New System.Windows.Forms.Label
        Me.b_load_other_2 = New System.Windows.Forms.Button
        Me.b_load_other_1 = New System.Windows.Forms.Button
        Me.Label1 = New System.Windows.Forms.Label
        Me.t_hostname = New System.Windows.Forms.TextBox
        Me.TabPage5 = New System.Windows.Forms.TabPage
        Me.b_demo7_load_scene = New System.Windows.Forms.Button
        Me.s_demo7_sfactorRGB = New System.Windows.Forms.ListBox
        Me.Label13 = New System.Windows.Forms.Label
        Me.Label14 = New System.Windows.Forms.Label
        Me.s_demo7_dfactorRGB = New System.Windows.Forms.ListBox
        Me.Label15 = New System.Windows.Forms.Label
        Me.s_demo7_sfactorAlpha = New System.Windows.Forms.ListBox
        Me.Label16 = New System.Windows.Forms.Label
        Me.s_demo7_dfactorAlpha = New System.Windows.Forms.ListBox
        Me.b_demo7_BlendFuncSeparateEXT = New System.Windows.Forms.Button
        Me.TabControl1.SuspendLayout()
        Me.TabPage1.SuspendLayout()
        Me.TabPage2.SuspendLayout()
        Me.TabPage3.SuspendLayout()
        Me.TabPage4.SuspendLayout()
        Me.TabPage5.SuspendLayout()
        Me.SuspendLayout()
        '
        'TabControl1
        '
        Me.TabControl1.Controls.Add(Me.TabPage1)
        Me.TabControl1.Controls.Add(Me.TabPage2)
        Me.TabControl1.Controls.Add(Me.TabPage3)
        Me.TabControl1.Controls.Add(Me.TabPage4)
        Me.TabControl1.Controls.Add(Me.TabPage5)
        Me.TabControl1.Location = New System.Drawing.Point(12, 49)
        Me.TabControl1.Name = "TabControl1"
        Me.TabControl1.SelectedIndex = 0
        Me.TabControl1.Size = New System.Drawing.Size(593, 378)
        Me.TabControl1.TabIndex = 0
        '
        'TabPage1
        '
        Me.TabPage1.Controls.Add(Me.b_demo1_director_reset)
        Me.TabPage1.Controls.Add(Me.b_demo1_director_continue)
        Me.TabPage1.Controls.Add(Me.b_demo1_director_start)
        Me.TabPage1.Controls.Add(Me.b_demo1_load_text)
        Me.TabPage1.Controls.Add(Me.t_demo1_t2)
        Me.TabPage1.Controls.Add(Me.Label3)
        Me.TabPage1.Controls.Add(Me.t_demo1_t1)
        Me.TabPage1.Controls.Add(Me.Label2)
        Me.TabPage1.Controls.Add(Me.b_demo1_load_scene)
        Me.TabPage1.Location = New System.Drawing.Point(4, 22)
        Me.TabPage1.Name = "TabPage1"
        Me.TabPage1.Padding = New System.Windows.Forms.Padding(3)
        Me.TabPage1.Size = New System.Drawing.Size(585, 352)
        Me.TabPage1.TabIndex = 0
        Me.TabPage1.Text = "demo1.xml"
        Me.TabPage1.UseVisualStyleBackColor = True
        '
        'b_demo1_director_reset
        '
        Me.b_demo1_director_reset.Location = New System.Drawing.Point(459, 121)
        Me.b_demo1_director_reset.Name = "b_demo1_director_reset"
        Me.b_demo1_director_reset.Size = New System.Drawing.Size(120, 23)
        Me.b_demo1_director_reset.TabIndex = 8
        Me.b_demo1_director_reset.Text = "Reset Animation"
        Me.b_demo1_director_reset.UseVisualStyleBackColor = True
        '
        'b_demo1_director_continue
        '
        Me.b_demo1_director_continue.Location = New System.Drawing.Point(345, 121)
        Me.b_demo1_director_continue.Name = "b_demo1_director_continue"
        Me.b_demo1_director_continue.Size = New System.Drawing.Size(108, 23)
        Me.b_demo1_director_continue.TabIndex = 7
        Me.b_demo1_director_continue.Text = "Continue Animation"
        Me.b_demo1_director_continue.UseVisualStyleBackColor = True
        '
        'b_demo1_director_start
        '
        Me.b_demo1_director_start.Location = New System.Drawing.Point(230, 121)
        Me.b_demo1_director_start.Name = "b_demo1_director_start"
        Me.b_demo1_director_start.Size = New System.Drawing.Size(109, 23)
        Me.b_demo1_director_start.TabIndex = 6
        Me.b_demo1_director_start.Text = "Start Animation"
        Me.b_demo1_director_start.UseVisualStyleBackColor = True
        '
        'b_demo1_load_text
        '
        Me.b_demo1_load_text.Location = New System.Drawing.Point(7, 121)
        Me.b_demo1_load_text.Name = "b_demo1_load_text"
        Me.b_demo1_load_text.Size = New System.Drawing.Size(75, 23)
        Me.b_demo1_load_text.TabIndex = 5
        Me.b_demo1_load_text.Text = "Load Texts"
        Me.b_demo1_load_text.UseVisualStyleBackColor = True
        '
        't_demo1_t2
        '
        Me.t_demo1_t2.Location = New System.Drawing.Point(7, 94)
        Me.t_demo1_t2.Name = "t_demo1_t2"
        Me.t_demo1_t2.Size = New System.Drawing.Size(572, 20)
        Me.t_demo1_t2.TabIndex = 4
        Me.t_demo1_t2.Text = "Vasilij Pupkin"
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Location = New System.Drawing.Point(7, 77)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(43, 13)
        Me.Label3.TabIndex = 3
        Me.Label3.Text = "Line #2"
        '
        't_demo1_t1
        '
        Me.t_demo1_t1.Location = New System.Drawing.Point(7, 53)
        Me.t_demo1_t1.Name = "t_demo1_t1"
        Me.t_demo1_t1.Size = New System.Drawing.Size(572, 20)
        Me.t_demo1_t1.TabIndex = 2
        Me.t_demo1_t1.Text = "Senior Deputy Officer"
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Location = New System.Drawing.Point(7, 36)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(43, 13)
        Me.Label2.TabIndex = 1
        Me.Label2.Text = "Line #1"
        '
        'b_demo1_load_scene
        '
        Me.b_demo1_load_scene.Location = New System.Drawing.Point(3, 6)
        Me.b_demo1_load_scene.Name = "b_demo1_load_scene"
        Me.b_demo1_load_scene.Size = New System.Drawing.Size(75, 23)
        Me.b_demo1_load_scene.TabIndex = 0
        Me.b_demo1_load_scene.Text = "Load scene"
        Me.b_demo1_load_scene.UseVisualStyleBackColor = True
        '
        'TabPage2
        '
        Me.TabPage2.Controls.Add(Me.b_demo3_speed30)
        Me.TabPage2.Controls.Add(Me.b_demo3_speed25)
        Me.TabPage2.Controls.Add(Me.b_demo3_speed20)
        Me.TabPage2.Controls.Add(Me.b_demo3_speed15)
        Me.TabPage2.Controls.Add(Me.b_demo3_speed10)
        Me.TabPage2.Controls.Add(Me.Label5)
        Me.TabPage2.Controls.Add(Me.b_demo3_reset)
        Me.TabPage2.Controls.Add(Me.b_demo3_loop_off)
        Me.TabPage2.Controls.Add(Me.b_demo3_loop_on)
        Me.TabPage2.Controls.Add(Me.b_demo3_append)
        Me.TabPage2.Controls.Add(Me.t_demo3_text)
        Me.TabPage2.Controls.Add(Me.Label4)
        Me.TabPage2.Controls.Add(Me.b_demo3_load_scene)
        Me.TabPage2.Location = New System.Drawing.Point(4, 22)
        Me.TabPage2.Name = "TabPage2"
        Me.TabPage2.Padding = New System.Windows.Forms.Padding(3)
        Me.TabPage2.Size = New System.Drawing.Size(585, 352)
        Me.TabPage2.TabIndex = 1
        Me.TabPage2.Text = "demo3.xml"
        Me.TabPage2.UseVisualStyleBackColor = True
        '
        'b_demo3_speed30
        '
        Me.b_demo3_speed30.Location = New System.Drawing.Point(186, 134)
        Me.b_demo3_speed30.Name = "b_demo3_speed30"
        Me.b_demo3_speed30.Size = New System.Drawing.Size(38, 23)
        Me.b_demo3_speed30.TabIndex = 12
        Me.b_demo3_speed30.Text = "3.0"
        Me.b_demo3_speed30.UseVisualStyleBackColor = True
        '
        'b_demo3_speed25
        '
        Me.b_demo3_speed25.Location = New System.Drawing.Point(142, 134)
        Me.b_demo3_speed25.Name = "b_demo3_speed25"
        Me.b_demo3_speed25.Size = New System.Drawing.Size(38, 23)
        Me.b_demo3_speed25.TabIndex = 11
        Me.b_demo3_speed25.Text = "2.5"
        Me.b_demo3_speed25.UseVisualStyleBackColor = True
        '
        'b_demo3_speed20
        '
        Me.b_demo3_speed20.Location = New System.Drawing.Point(98, 134)
        Me.b_demo3_speed20.Name = "b_demo3_speed20"
        Me.b_demo3_speed20.Size = New System.Drawing.Size(38, 23)
        Me.b_demo3_speed20.TabIndex = 10
        Me.b_demo3_speed20.Text = "2.0"
        Me.b_demo3_speed20.UseVisualStyleBackColor = True
        '
        'b_demo3_speed15
        '
        Me.b_demo3_speed15.Location = New System.Drawing.Point(54, 134)
        Me.b_demo3_speed15.Name = "b_demo3_speed15"
        Me.b_demo3_speed15.Size = New System.Drawing.Size(38, 23)
        Me.b_demo3_speed15.TabIndex = 9
        Me.b_demo3_speed15.Text = "1.5"
        Me.b_demo3_speed15.UseVisualStyleBackColor = True
        '
        'b_demo3_speed10
        '
        Me.b_demo3_speed10.Location = New System.Drawing.Point(10, 134)
        Me.b_demo3_speed10.Name = "b_demo3_speed10"
        Me.b_demo3_speed10.Size = New System.Drawing.Size(38, 23)
        Me.b_demo3_speed10.TabIndex = 8
        Me.b_demo3_speed10.Text = "1.0"
        Me.b_demo3_speed10.UseVisualStyleBackColor = True
        '
        'Label5
        '
        Me.Label5.AutoSize = True
        Me.Label5.Location = New System.Drawing.Point(7, 117)
        Me.Label5.Name = "Label5"
        Me.Label5.Size = New System.Drawing.Size(73, 13)
        Me.Label5.TabIndex = 7
        Me.Label5.Text = "Speed control"
        '
        'b_demo3_reset
        '
        Me.b_demo3_reset.Location = New System.Drawing.Point(503, 81)
        Me.b_demo3_reset.Name = "b_demo3_reset"
        Me.b_demo3_reset.Size = New System.Drawing.Size(75, 23)
        Me.b_demo3_reset.TabIndex = 6
        Me.b_demo3_reset.Text = "Reset"
        Me.b_demo3_reset.UseVisualStyleBackColor = True
        '
        'b_demo3_loop_off
        '
        Me.b_demo3_loop_off.Location = New System.Drawing.Point(421, 81)
        Me.b_demo3_loop_off.Name = "b_demo3_loop_off"
        Me.b_demo3_loop_off.Size = New System.Drawing.Size(75, 23)
        Me.b_demo3_loop_off.TabIndex = 5
        Me.b_demo3_loop_off.Text = "Loop OFF"
        Me.b_demo3_loop_off.UseVisualStyleBackColor = True
        '
        'b_demo3_loop_on
        '
        Me.b_demo3_loop_on.Location = New System.Drawing.Point(339, 81)
        Me.b_demo3_loop_on.Name = "b_demo3_loop_on"
        Me.b_demo3_loop_on.Size = New System.Drawing.Size(75, 23)
        Me.b_demo3_loop_on.TabIndex = 4
        Me.b_demo3_loop_on.Text = "Loop ON"
        Me.b_demo3_loop_on.UseVisualStyleBackColor = True
        '
        'b_demo3_append
        '
        Me.b_demo3_append.Location = New System.Drawing.Point(10, 81)
        Me.b_demo3_append.Name = "b_demo3_append"
        Me.b_demo3_append.Size = New System.Drawing.Size(90, 23)
        Me.b_demo3_append.TabIndex = 3
        Me.b_demo3_append.Text = "Append Text"
        Me.b_demo3_append.UseVisualStyleBackColor = True
        '
        't_demo3_text
        '
        Me.t_demo3_text.Location = New System.Drawing.Point(10, 54)
        Me.t_demo3_text.Name = "t_demo3_text"
        Me.t_demo3_text.Size = New System.Drawing.Size(569, 20)
        Me.t_demo3_text.TabIndex = 2
        Me.t_demo3_text.Text = "NEWS: VZ has a simple VB2008 integration"
        '
        'Label4
        '
        Me.Label4.AutoSize = True
        Me.Label4.Location = New System.Drawing.Point(7, 37)
        Me.Label4.Name = "Label4"
        Me.Label4.Size = New System.Drawing.Size(83, 13)
        Me.Label4.TabIndex = 1
        Me.Label4.Text = "Text to Append:"
        '
        'b_demo3_load_scene
        '
        Me.b_demo3_load_scene.Location = New System.Drawing.Point(7, 7)
        Me.b_demo3_load_scene.Name = "b_demo3_load_scene"
        Me.b_demo3_load_scene.Size = New System.Drawing.Size(75, 23)
        Me.b_demo3_load_scene.TabIndex = 0
        Me.b_demo3_load_scene.Text = "Load Scene"
        Me.b_demo3_load_scene.UseVisualStyleBackColor = True
        '
        'TabPage3
        '
        Me.TabPage3.Controls.Add(Me.b_demo5_set_stop)
        Me.TabPage3.Controls.Add(Me.b_demo5_set_start)
        Me.TabPage3.Controls.Add(Me.t_demo5_stop_value)
        Me.TabPage3.Controls.Add(Me.t_demo5_start_value)
        Me.TabPage3.Controls.Add(Me.Label10)
        Me.TabPage3.Controls.Add(Me.Label9)
        Me.TabPage3.Controls.Add(Me.Label8)
        Me.TabPage3.Controls.Add(Me.b_demo5_cont)
        Me.TabPage3.Controls.Add(Me.b_demo5_reset)
        Me.TabPage3.Controls.Add(Me.b_demo5_stop)
        Me.TabPage3.Controls.Add(Me.b_demo5_start)
        Me.TabPage3.Controls.Add(Me.Label7)
        Me.TabPage3.Controls.Add(Me.b_demo5_f2)
        Me.TabPage3.Controls.Add(Me.b_demo5_f3)
        Me.TabPage3.Controls.Add(Me.b_demo5_f1)
        Me.TabPage3.Controls.Add(Me.Label6)
        Me.TabPage3.Controls.Add(Me.b_demo5_load_scene)
        Me.TabPage3.Location = New System.Drawing.Point(4, 22)
        Me.TabPage3.Name = "TabPage3"
        Me.TabPage3.Padding = New System.Windows.Forms.Padding(3)
        Me.TabPage3.Size = New System.Drawing.Size(585, 352)
        Me.TabPage3.TabIndex = 2
        Me.TabPage3.Text = "demo5.xml"
        Me.TabPage3.UseVisualStyleBackColor = True
        '
        'b_demo5_set_stop
        '
        Me.b_demo5_set_stop.Location = New System.Drawing.Point(199, 229)
        Me.b_demo5_set_stop.Name = "b_demo5_set_stop"
        Me.b_demo5_set_stop.Size = New System.Drawing.Size(53, 23)
        Me.b_demo5_set_stop.TabIndex = 14
        Me.b_demo5_set_stop.Text = "SET"
        Me.b_demo5_set_stop.UseVisualStyleBackColor = True
        '
        'b_demo5_set_start
        '
        Me.b_demo5_set_start.Location = New System.Drawing.Point(199, 201)
        Me.b_demo5_set_start.Name = "b_demo5_set_start"
        Me.b_demo5_set_start.Size = New System.Drawing.Size(53, 23)
        Me.b_demo5_set_start.TabIndex = 14
        Me.b_demo5_set_start.Text = "SET"
        Me.b_demo5_set_start.UseVisualStyleBackColor = True
        '
        't_demo5_stop_value
        '
        Me.t_demo5_stop_value.Location = New System.Drawing.Point(93, 231)
        Me.t_demo5_stop_value.Name = "t_demo5_stop_value"
        Me.t_demo5_stop_value.Size = New System.Drawing.Size(100, 20)
        Me.t_demo5_stop_value.TabIndex = 13
        Me.t_demo5_stop_value.Text = "20000"
        '
        't_demo5_start_value
        '
        Me.t_demo5_start_value.Location = New System.Drawing.Point(93, 203)
        Me.t_demo5_start_value.Name = "t_demo5_start_value"
        Me.t_demo5_start_value.Size = New System.Drawing.Size(100, 20)
        Me.t_demo5_start_value.TabIndex = 13
        Me.t_demo5_start_value.Text = "10000"
        '
        'Label10
        '
        Me.Label10.AutoSize = True
        Me.Label10.Location = New System.Drawing.Point(7, 234)
        Me.Label10.Name = "Label10"
        Me.Label10.Size = New System.Drawing.Size(62, 13)
        Me.Label10.TabIndex = 12
        Me.Label10.Text = "Stop Value:"
        '
        'Label9
        '
        Me.Label9.AutoSize = True
        Me.Label9.Location = New System.Drawing.Point(7, 206)
        Me.Label9.Name = "Label9"
        Me.Label9.Size = New System.Drawing.Size(62, 13)
        Me.Label9.TabIndex = 12
        Me.Label9.Text = "Start Value:"
        '
        'Label8
        '
        Me.Label8.AutoSize = True
        Me.Label8.Location = New System.Drawing.Point(6, 182)
        Me.Label8.Name = "Label8"
        Me.Label8.Size = New System.Drawing.Size(126, 13)
        Me.Label8.TabIndex = 11
        Me.Label8.Text = "Timer range (miliseconds)"
        '
        'b_demo5_cont
        '
        Me.b_demo5_cont.Location = New System.Drawing.Point(169, 134)
        Me.b_demo5_cont.Name = "b_demo5_cont"
        Me.b_demo5_cont.Size = New System.Drawing.Size(75, 23)
        Me.b_demo5_cont.TabIndex = 10
        Me.b_demo5_cont.Text = "Continue"
        Me.b_demo5_cont.UseVisualStyleBackColor = True
        '
        'b_demo5_reset
        '
        Me.b_demo5_reset.Location = New System.Drawing.Point(250, 135)
        Me.b_demo5_reset.Name = "b_demo5_reset"
        Me.b_demo5_reset.Size = New System.Drawing.Size(75, 23)
        Me.b_demo5_reset.TabIndex = 9
        Me.b_demo5_reset.Text = "Reset"
        Me.b_demo5_reset.UseVisualStyleBackColor = True
        '
        'b_demo5_stop
        '
        Me.b_demo5_stop.Location = New System.Drawing.Point(87, 135)
        Me.b_demo5_stop.Name = "b_demo5_stop"
        Me.b_demo5_stop.Size = New System.Drawing.Size(75, 23)
        Me.b_demo5_stop.TabIndex = 8
        Me.b_demo5_stop.Text = "Stop"
        Me.b_demo5_stop.UseVisualStyleBackColor = True
        '
        'b_demo5_start
        '
        Me.b_demo5_start.Location = New System.Drawing.Point(6, 135)
        Me.b_demo5_start.Name = "b_demo5_start"
        Me.b_demo5_start.Size = New System.Drawing.Size(75, 23)
        Me.b_demo5_start.TabIndex = 7
        Me.b_demo5_start.Text = "Start"
        Me.b_demo5_start.UseVisualStyleBackColor = True
        '
        'Label7
        '
        Me.Label7.AutoSize = True
        Me.Label7.Location = New System.Drawing.Point(3, 114)
        Me.Label7.Name = "Label7"
        Me.Label7.Size = New System.Drawing.Size(105, 13)
        Me.Label7.TabIndex = 6
        Me.Label7.Text = "Timer runtime control"
        '
        'b_demo5_f2
        '
        Me.b_demo5_f2.Location = New System.Drawing.Point(110, 69)
        Me.b_demo5_f2.Name = "b_demo5_f2"
        Me.b_demo5_f2.Size = New System.Drawing.Size(75, 23)
        Me.b_demo5_f2.TabIndex = 5
        Me.b_demo5_f2.Text = "m:SS"
        Me.b_demo5_f2.UseVisualStyleBackColor = True
        '
        'b_demo5_f3
        '
        Me.b_demo5_f3.Location = New System.Drawing.Point(191, 70)
        Me.b_demo5_f3.Name = "b_demo5_f3"
        Me.b_demo5_f3.Size = New System.Drawing.Size(88, 23)
        Me.b_demo5_f3.TabIndex = 4
        Me.b_demo5_f3.Text = "HH:MM:SS:FF"
        Me.b_demo5_f3.UseVisualStyleBackColor = True
        '
        'b_demo5_f1
        '
        Me.b_demo5_f1.Location = New System.Drawing.Point(6, 70)
        Me.b_demo5_f1.Name = "b_demo5_f1"
        Me.b_demo5_f1.Size = New System.Drawing.Size(97, 23)
        Me.b_demo5_f1.TabIndex = 3
        Me.b_demo5_f1.Text = "MM:SS.DDD"
        Me.b_demo5_f1.UseVisualStyleBackColor = True
        '
        'Label6
        '
        Me.Label6.AutoSize = True
        Me.Label6.Location = New System.Drawing.Point(7, 53)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(73, 13)
        Me.Label6.TabIndex = 2
        Me.Label6.Text = "Timer formats:"
        '
        'b_demo5_load_scene
        '
        Me.b_demo5_load_scene.Location = New System.Drawing.Point(6, 6)
        Me.b_demo5_load_scene.Name = "b_demo5_load_scene"
        Me.b_demo5_load_scene.Size = New System.Drawing.Size(75, 23)
        Me.b_demo5_load_scene.TabIndex = 1
        Me.b_demo5_load_scene.Text = "Load Scene"
        Me.b_demo5_load_scene.UseVisualStyleBackColor = True
        '
        'TabPage4
        '
        Me.TabPage4.Controls.Add(Me.Label12)
        Me.TabPage4.Controls.Add(Me.Label11)
        Me.TabPage4.Controls.Add(Me.b_load_other_2)
        Me.TabPage4.Controls.Add(Me.b_load_other_1)
        Me.TabPage4.Location = New System.Drawing.Point(4, 22)
        Me.TabPage4.Name = "TabPage4"
        Me.TabPage4.Padding = New System.Windows.Forms.Padding(3)
        Me.TabPage4.Size = New System.Drawing.Size(585, 352)
        Me.TabPage4.TabIndex = 3
        Me.TabPage4.Text = "Other"
        Me.TabPage4.UseVisualStyleBackColor = True
        '
        'Label12
        '
        Me.Label12.AutoSize = True
        Me.Label12.Location = New System.Drawing.Point(150, 40)
        Me.Label12.Name = "Label12"
        Me.Label12.Size = New System.Drawing.Size(103, 13)
        Me.Label12.TabIndex = 1
        Me.Label12.Text = "Image free transform"
        '
        'Label11
        '
        Me.Label11.AutoSize = True
        Me.Label11.Location = New System.Drawing.Point(150, 11)
        Me.Label11.Name = "Label11"
        Me.Label11.Size = New System.Drawing.Size(83, 13)
        Me.Label11.TabIndex = 1
        Me.Label11.Text = "Trajectory demo"
        '
        'b_load_other_2
        '
        Me.b_load_other_2.Location = New System.Drawing.Point(6, 35)
        Me.b_load_other_2.Name = "b_load_other_2"
        Me.b_load_other_2.Size = New System.Drawing.Size(138, 23)
        Me.b_load_other_2.TabIndex = 0
        Me.b_load_other_2.Text = "image_test_ft.xml"
        Me.b_load_other_2.UseVisualStyleBackColor = True
        '
        'b_load_other_1
        '
        Me.b_load_other_1.Location = New System.Drawing.Point(6, 6)
        Me.b_load_other_1.Name = "b_load_other_1"
        Me.b_load_other_1.Size = New System.Drawing.Size(138, 23)
        Me.b_load_other_1.TabIndex = 0
        Me.b_load_other_1.Text = "cube.xml"
        Me.b_load_other_1.UseVisualStyleBackColor = True
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Location = New System.Drawing.Point(12, 12)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(92, 13)
        Me.Label1.TabIndex = 1
        Me.Label1.Text = "Host name (or IP):"
        '
        't_hostname
        '
        Me.t_hostname.Location = New System.Drawing.Point(112, 9)
        Me.t_hostname.Name = "t_hostname"
        Me.t_hostname.Size = New System.Drawing.Size(188, 20)
        Me.t_hostname.TabIndex = 2
        Me.t_hostname.Text = "localhost"
        '
        'TabPage5
        '
        Me.TabPage5.Controls.Add(Me.b_demo7_BlendFuncSeparateEXT)
        Me.TabPage5.Controls.Add(Me.Label16)
        Me.TabPage5.Controls.Add(Me.s_demo7_dfactorAlpha)
        Me.TabPage5.Controls.Add(Me.Label15)
        Me.TabPage5.Controls.Add(Me.s_demo7_sfactorAlpha)
        Me.TabPage5.Controls.Add(Me.Label14)
        Me.TabPage5.Controls.Add(Me.s_demo7_dfactorRGB)
        Me.TabPage5.Controls.Add(Me.Label13)
        Me.TabPage5.Controls.Add(Me.s_demo7_sfactorRGB)
        Me.TabPage5.Controls.Add(Me.b_demo7_load_scene)
        Me.TabPage5.Location = New System.Drawing.Point(4, 22)
        Me.TabPage5.Name = "TabPage5"
        Me.TabPage5.Padding = New System.Windows.Forms.Padding(3)
        Me.TabPage5.Size = New System.Drawing.Size(585, 352)
        Me.TabPage5.TabIndex = 4
        Me.TabPage5.Text = "demo7.xml"
        Me.TabPage5.UseVisualStyleBackColor = True
        '
        'b_demo7_load_scene
        '
        Me.b_demo7_load_scene.Location = New System.Drawing.Point(6, 6)
        Me.b_demo7_load_scene.Name = "b_demo7_load_scene"
        Me.b_demo7_load_scene.Size = New System.Drawing.Size(106, 23)
        Me.b_demo7_load_scene.TabIndex = 0
        Me.b_demo7_load_scene.Text = "Load scene"
        Me.b_demo7_load_scene.UseVisualStyleBackColor = True
        '
        's_demo7_sfactorRGB
        '
        Me.s_demo7_sfactorRGB.FormattingEnabled = True
        Me.s_demo7_sfactorRGB.Items.AddRange(New Object() {"ZERO", "ONE", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA", "CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA", "SRC_ALPHA_SATURATE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR"})
        Me.s_demo7_sfactorRGB.Location = New System.Drawing.Point(6, 45)
        Me.s_demo7_sfactorRGB.Name = "s_demo7_sfactorRGB"
        Me.s_demo7_sfactorRGB.ScrollAlwaysVisible = True
        Me.s_demo7_sfactorRGB.Size = New System.Drawing.Size(278, 134)
        Me.s_demo7_sfactorRGB.TabIndex = 1
        '
        'Label13
        '
        Me.Label13.AutoSize = True
        Me.Label13.Location = New System.Drawing.Point(6, 32)
        Me.Label13.Name = "Label13"
        Me.Label13.Size = New System.Drawing.Size(62, 13)
        Me.Label13.TabIndex = 2
        Me.Label13.Text = "sfactorRGB"
        '
        'Label14
        '
        Me.Label14.AutoSize = True
        Me.Label14.Location = New System.Drawing.Point(301, 32)
        Me.Label14.Name = "Label14"
        Me.Label14.Size = New System.Drawing.Size(63, 13)
        Me.Label14.TabIndex = 4
        Me.Label14.Text = "dfactorRGB"
        '
        's_demo7_dfactorRGB
        '
        Me.s_demo7_dfactorRGB.FormattingEnabled = True
        Me.s_demo7_dfactorRGB.Items.AddRange(New Object() {"ZERO", "ONE", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA", "CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA", "SRC_ALPHA_SATURATE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR"})
        Me.s_demo7_dfactorRGB.Location = New System.Drawing.Point(301, 45)
        Me.s_demo7_dfactorRGB.Name = "s_demo7_dfactorRGB"
        Me.s_demo7_dfactorRGB.Size = New System.Drawing.Size(278, 134)
        Me.s_demo7_dfactorRGB.TabIndex = 3
        '
        'Label15
        '
        Me.Label15.AutoSize = True
        Me.Label15.Location = New System.Drawing.Point(3, 189)
        Me.Label15.Name = "Label15"
        Me.Label15.Size = New System.Drawing.Size(66, 13)
        Me.Label15.TabIndex = 6
        Me.Label15.Text = "sfactorAlpha"
        '
        's_demo7_sfactorAlpha
        '
        Me.s_demo7_sfactorAlpha.FormattingEnabled = True
        Me.s_demo7_sfactorAlpha.Items.AddRange(New Object() {"ZERO", "ONE", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA", "CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA", "SRC_ALPHA_SATURATE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR"})
        Me.s_demo7_sfactorAlpha.Location = New System.Drawing.Point(6, 205)
        Me.s_demo7_sfactorAlpha.Name = "s_demo7_sfactorAlpha"
        Me.s_demo7_sfactorAlpha.Size = New System.Drawing.Size(278, 147)
        Me.s_demo7_sfactorAlpha.TabIndex = 5
        '
        'Label16
        '
        Me.Label16.AutoSize = True
        Me.Label16.Location = New System.Drawing.Point(298, 189)
        Me.Label16.Name = "Label16"
        Me.Label16.Size = New System.Drawing.Size(67, 13)
        Me.Label16.TabIndex = 8
        Me.Label16.Text = "dfactorAlpha"
        '
        's_demo7_dfactorAlpha
        '
        Me.s_demo7_dfactorAlpha.FormattingEnabled = True
        Me.s_demo7_dfactorAlpha.Items.AddRange(New Object() {"ZERO", "ONE", "DST_COLOR", "ONE_MINUS_DST_COLOR", "SRC_ALPHA", "ONE_MINUS_SRC_ALPHA", "DST_ALPHA", "ONE_MINUS_DST_ALPHA", "CONSTANT_COLOR", "ONE_MINUS_CONSTANT_COLOR", "CONSTANT_ALPHA", "ONE_MINUS_CONSTANT_ALPHA", "SRC_ALPHA_SATURATE", "SRC_COLOR", "ONE_MINUS_SRC_COLOR"})
        Me.s_demo7_dfactorAlpha.Location = New System.Drawing.Point(301, 205)
        Me.s_demo7_dfactorAlpha.Name = "s_demo7_dfactorAlpha"
        Me.s_demo7_dfactorAlpha.Size = New System.Drawing.Size(278, 147)
        Me.s_demo7_dfactorAlpha.TabIndex = 7
        '
        'b_demo7_BlendFuncSeparateEXT
        '
        Me.b_demo7_BlendFuncSeparateEXT.Location = New System.Drawing.Point(405, 6)
        Me.b_demo7_BlendFuncSeparateEXT.Name = "b_demo7_BlendFuncSeparateEXT"
        Me.b_demo7_BlendFuncSeparateEXT.Size = New System.Drawing.Size(173, 23)
        Me.b_demo7_BlendFuncSeparateEXT.TabIndex = 9
        Me.b_demo7_BlendFuncSeparateEXT.Text = "set BlendFuncSeparateEXT"
        Me.b_demo7_BlendFuncSeparateEXT.UseVisualStyleBackColor = True
        '
        'MainForm
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(617, 458)
        Me.Controls.Add(Me.t_hostname)
        Me.Controls.Add(Me.Label1)
        Me.Controls.Add(Me.TabControl1)
        Me.Name = "MainForm"
        Me.Text = "VZ DEMO PILOT (VB2008)"
        Me.TabControl1.ResumeLayout(False)
        Me.TabPage1.ResumeLayout(False)
        Me.TabPage1.PerformLayout()
        Me.TabPage2.ResumeLayout(False)
        Me.TabPage2.PerformLayout()
        Me.TabPage3.ResumeLayout(False)
        Me.TabPage3.PerformLayout()
        Me.TabPage4.ResumeLayout(False)
        Me.TabPage4.PerformLayout()
        Me.TabPage5.ResumeLayout(False)
        Me.TabPage5.PerformLayout()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents TabControl1 As System.Windows.Forms.TabControl
    Friend WithEvents TabPage1 As System.Windows.Forms.TabPage
    Friend WithEvents TabPage2 As System.Windows.Forms.TabPage
    Friend WithEvents TabPage3 As System.Windows.Forms.TabPage
    Friend WithEvents Label1 As System.Windows.Forms.Label
    Friend WithEvents t_hostname As System.Windows.Forms.TextBox
    Friend WithEvents b_demo1_load_scene As System.Windows.Forms.Button
    Friend WithEvents t_demo1_t2 As System.Windows.Forms.TextBox
    Friend WithEvents Label3 As System.Windows.Forms.Label
    Friend WithEvents t_demo1_t1 As System.Windows.Forms.TextBox
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents b_demo1_load_text As System.Windows.Forms.Button
    Friend WithEvents b_demo1_director_reset As System.Windows.Forms.Button
    Friend WithEvents b_demo1_director_continue As System.Windows.Forms.Button
    Friend WithEvents b_demo1_director_start As System.Windows.Forms.Button
    Friend WithEvents b_demo3_load_scene As System.Windows.Forms.Button
    Friend WithEvents t_demo3_text As System.Windows.Forms.TextBox
    Friend WithEvents Label4 As System.Windows.Forms.Label
    Friend WithEvents b_demo3_speed15 As System.Windows.Forms.Button
    Friend WithEvents b_demo3_speed10 As System.Windows.Forms.Button
    Friend WithEvents Label5 As System.Windows.Forms.Label
    Friend WithEvents b_demo3_reset As System.Windows.Forms.Button
    Friend WithEvents b_demo3_loop_off As System.Windows.Forms.Button
    Friend WithEvents b_demo3_loop_on As System.Windows.Forms.Button
    Friend WithEvents b_demo3_append As System.Windows.Forms.Button
    Friend WithEvents b_demo3_speed30 As System.Windows.Forms.Button
    Friend WithEvents b_demo3_speed25 As System.Windows.Forms.Button
    Friend WithEvents b_demo3_speed20 As System.Windows.Forms.Button
    Friend WithEvents b_demo5_load_scene As System.Windows.Forms.Button
    Friend WithEvents b_demo5_cont As System.Windows.Forms.Button
    Friend WithEvents b_demo5_reset As System.Windows.Forms.Button
    Friend WithEvents b_demo5_stop As System.Windows.Forms.Button
    Friend WithEvents b_demo5_start As System.Windows.Forms.Button
    Friend WithEvents Label7 As System.Windows.Forms.Label
    Friend WithEvents b_demo5_f2 As System.Windows.Forms.Button
    Friend WithEvents b_demo5_f3 As System.Windows.Forms.Button
    Friend WithEvents b_demo5_f1 As System.Windows.Forms.Button
    Friend WithEvents Label6 As System.Windows.Forms.Label
    Friend WithEvents Label8 As System.Windows.Forms.Label
    Friend WithEvents b_demo5_set_start As System.Windows.Forms.Button
    Friend WithEvents t_demo5_start_value As System.Windows.Forms.TextBox
    Friend WithEvents Label9 As System.Windows.Forms.Label
    Friend WithEvents b_demo5_set_stop As System.Windows.Forms.Button
    Friend WithEvents t_demo5_stop_value As System.Windows.Forms.TextBox
    Friend WithEvents Label10 As System.Windows.Forms.Label
    Friend WithEvents TabPage4 As System.Windows.Forms.TabPage
    Friend WithEvents b_load_other_1 As System.Windows.Forms.Button
    Friend WithEvents Label12 As System.Windows.Forms.Label
    Friend WithEvents Label11 As System.Windows.Forms.Label
    Friend WithEvents b_load_other_2 As System.Windows.Forms.Button
    Friend WithEvents TabPage5 As System.Windows.Forms.TabPage
    Friend WithEvents s_demo7_sfactorRGB As System.Windows.Forms.ListBox
    Private WithEvents b_demo7_load_scene As System.Windows.Forms.Button
    Friend WithEvents Label13 As System.Windows.Forms.Label
    Friend WithEvents Label16 As System.Windows.Forms.Label
    Friend WithEvents s_demo7_dfactorAlpha As System.Windows.Forms.ListBox
    Friend WithEvents Label15 As System.Windows.Forms.Label
    Friend WithEvents s_demo7_sfactorAlpha As System.Windows.Forms.ListBox
    Friend WithEvents Label14 As System.Windows.Forms.Label
    Friend WithEvents s_demo7_dfactorRGB As System.Windows.Forms.ListBox
    Friend WithEvents b_demo7_BlendFuncSeparateEXT As System.Windows.Forms.Button

End Class
