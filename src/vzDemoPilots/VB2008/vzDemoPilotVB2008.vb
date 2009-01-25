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

    Private Sub b_demo3_load_scene_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo3_load_scene.Click
        Dim argv(0 To 1) As String

        argv(0) = VZ_CMD_LOAD_SCENE
        argv(1) = "./projects/demo3.xml"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo3_append_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo3_append.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "text_crawl"
        argv(2) = "s_trig_append"
        argv(3) = t_demo3_text.Text

        send_vz_data(argv)
    End Sub

    Private Sub b_demo3_loop_on_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo3_loop_on.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "text_crawl"
        argv(2) = "l_loop"
        argv(3) = "1"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo3_loop_off_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo3_loop_off.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "text_crawl"
        argv(2) = "l_loop"
        argv(3) = "0"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo3_reset_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo3_reset.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "text_crawl"
        argv(2) = "l_reset"
        argv(3) = "1"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo3_speed10_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo3_speed10.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "text_crawl"
        argv(2) = "f_speed"
        argv(3) = "1.0"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo3_speed15_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo3_speed15.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "text_crawl"
        argv(2) = "f_speed"
        argv(3) = "1.5"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo3_speed20_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo3_speed20.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "text_crawl"
        argv(2) = "f_speed"
        argv(3) = "2.0"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo3_speed25_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo3_speed25.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "text_crawl"
        argv(2) = "f_speed"
        argv(3) = "2.5"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo3_speed30_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo3_speed30.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "text_crawl"
        argv(2) = "f_speed"
        argv(3) = "3.0"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo5_load_scene_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo5_load_scene.Click
        Dim argv(0 To 1) As String

        argv(0) = VZ_CMD_LOAD_SCENE
        argv(1) = "./projects/demo5.xml"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo5_f1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo5_f1.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "timer1"
        argv(2) = "s_format"
        argv(3) = "MM:SS.DDD"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo5_f2_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo5_f2.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "timer1"
        argv(2) = "s_format"
        argv(3) = "m:SS"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo5_f3_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo5_f3.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "timer1"
        argv(2) = "s_format"
        argv(3) = "HH:MM:SS:FF"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo5_start_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo5_start.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "timer1"
        argv(2) = "l_trig_start"
        argv(3) = "1"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo5_stop_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo5_stop.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "timer1"
        argv(2) = "l_trig_stop"
        argv(3) = "1"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo5_cont_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo5_cont.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "timer1"
        argv(2) = "l_trig_cont"
        argv(3) = "1"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo5_reset_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo5_reset.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "timer1"
        argv(2) = "l_trig_reset"
        argv(3) = "1"

        send_vz_data(argv)
    End Sub

    Private Sub b_demo5_set_start_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo5_set_start.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "timer1"
        argv(2) = "l_start_value"
        argv(3) = t_demo5_start_value.Text

        send_vz_data(argv)
    End Sub

    Private Sub b_demo5_set_stop_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_demo5_set_stop.Click
        Dim argv(0 To 3) As String

        ' Setup command
        argv(0) = VZ_CMD_SET
        argv(1) = "timer1"
        argv(2) = "l_stop_value"
        argv(3) = t_demo5_stop_value.Text

        send_vz_data(argv)
    End Sub

    Private Sub b_load_other_1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_load_other_1.Click
        Dim argv(0 To 1) As String

        argv(0) = VZ_CMD_LOAD_SCENE
        argv(1) = "./projects/cube.xml"

        send_vz_data(argv)
    End Sub

    Private Sub b_load_other_2_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles b_load_other_2.Click
        Dim argv(0 To 1) As String

        argv(0) = VZ_CMD_LOAD_SCENE
        argv(1) = "./projects/image_test_ft.xml"

        send_vz_data(argv)
    End Sub
End Class
