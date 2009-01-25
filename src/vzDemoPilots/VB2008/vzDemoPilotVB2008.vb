Public Class MainForm
    Private Sub send_vz_data(ByVal argv() As String)
        Dim r As Long
        Dim count As Long
        Dim start As Long

        ' calc count of elements
        start = LBound(argv)
        count = UBound(argv) - start + 1

        ' Send command
        r = vzCmdSendVB(t_hostname.Text, count, argv)

    End Sub

    Private Sub b_demo1_load_scene_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo1_load_scene.Click
        Dim argv(0 To 1) As String

        argv(0) = VZ_CMD_LOAD_SCENE
        argv(1) = "./projects/demo1.xml"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo1_load_text_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo1_load_text.Click
        Dim argv(0 To 7) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "text_1"
        argv(2) = "s_text"
        argv(3) = t_demo1_t1.Text
        argv(4) = VZ_CMD_SET
        argv(5) = "text_2"
        argv(6) = "s_text"
        argv(7) = t_demo1_t2.Text

        send_vz_data(argv)

    End Sub

    Private Sub b_demo1_director_start_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo1_director_start.Click
        Dim argv(0 To 2) As String

        ' Setup command
        argv(0) = VZ_CMD_START_DIRECTOR
        argv(1) = "main"
        argv(2) = "0"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo1_director_reset_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo1_director_reset.Click
        Dim argv(0 To 2) As String

        ' Setup command
        argv(0) = VZ_CMD_RESET_DIRECTOR
        argv(1) = "main"
        argv(2) = "0"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo1_director_continue_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo1_director_continue.Click
        Dim argv(0 To 1) As String

        ' Setup command
        argv(0) = VZ_CMD_CONTINUE_DIRECTOR
        argv(1) = "main"

        send_vz_data(argv)
    End Sub
End Class
