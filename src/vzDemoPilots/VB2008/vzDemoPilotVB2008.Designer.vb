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
        Me.t_demo1_t2 = New System.Windows.Forms.TextBox
        Me.Label3 = New System.Windows.Forms.Label
        Me.t_demo1_t1 = New System.Windows.Forms.TextBox
        Me.Label2 = New System.Windows.Forms.Label
        Me.b_demo1_load_scene = New System.Windows.Forms.Button
        Me.TabPage2 = New System.Windows.Forms.TabPage
        Me.TabPage3 = New System.Windows.Forms.TabPage
        Me.Label1 = New System.Windows.Forms.Label
        Me.t_hostname = New System.Windows.Forms.TextBox
        Me.b_demo1_load_text = New System.Windows.Forms.Button
        Me.b_demo1_director_start = New System.Windows.Forms.Button
        Me.b_demo1_director_continue = New System.Windows.Forms.Button
        Me.b_demo1_director_reset = New System.Windows.Forms.Button
        Me.TabControl1.SuspendLayout()
        Me.TabPage1.SuspendLayout()
        Me.SuspendLayout()
        '
        'TabControl1
        '
        Me.TabControl1.Controls.Add(Me.TabPage1)
        Me.TabControl1.Controls.Add(Me.TabPage2)
        Me.TabControl1.Controls.Add(Me.TabPage3)
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
        Me.TabPage2.Location = New System.Drawing.Point(4, 22)
        Me.TabPage2.Name = "TabPage2"
        Me.TabPage2.Padding = New System.Windows.Forms.Padding(3)
        Me.TabPage2.Size = New System.Drawing.Size(585, 352)
        Me.TabPage2.TabIndex = 1
        Me.TabPage2.Text = "TabPage2"
        Me.TabPage2.UseVisualStyleBackColor = True
        '
        'TabPage3
        '
        Me.TabPage3.Location = New System.Drawing.Point(4, 22)
        Me.TabPage3.Name = "TabPage3"
        Me.TabPage3.Padding = New System.Windows.Forms.Padding(3)
        Me.TabPage3.Size = New System.Drawing.Size(585, 352)
        Me.TabPage3.TabIndex = 2
        Me.TabPage3.Text = "TabPage3"
        Me.TabPage3.UseVisualStyleBackColor = True
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
        'b_demo1_load_text
        '
        Me.b_demo1_load_text.Location = New System.Drawing.Point(7, 121)
        Me.b_demo1_load_text.Name = "b_demo1_load_text"
        Me.b_demo1_load_text.Size = New System.Drawing.Size(75, 23)
        Me.b_demo1_load_text.TabIndex = 5
        Me.b_demo1_load_text.Text = "Load Texts"
        Me.b_demo1_load_text.UseVisualStyleBackColor = True
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
        'b_demo1_director_continue
        '
        Me.b_demo1_director_continue.Location = New System.Drawing.Point(345, 121)
        Me.b_demo1_director_continue.Name = "b_demo1_director_continue"
        Me.b_demo1_director_continue.Size = New System.Drawing.Size(108, 23)
        Me.b_demo1_director_continue.TabIndex = 7
        Me.b_demo1_director_continue.Text = "Continue Animation"
        Me.b_demo1_director_continue.UseVisualStyleBackColor = True
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

End Class
