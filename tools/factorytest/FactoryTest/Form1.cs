using FactoryTest.Properties;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Text;
using System.Windows.Forms;

namespace FactoryTest
{
    public partial class MainForm : Form
    {
        StreamWriter fs;
        Process process, resetprocess;
        Victor70C device;
        int timeout;

        enum Status
        {
            BurnFinish,
            TestFinish,
            Fail,
            Done
        }

        Status currentstatus;

        public MainForm()
        {
            InitializeComponent();

            timer2.Enabled = false;
            device = new Victor70C();
            try
            {
                device.Start();
            }
            catch (Exception e)
            {
                MessageBox.Show(e.Message);
            }
            OutputPathTextBox.Text = Path.GetDirectoryName(Assembly.GetEntryAssembly().Location);
            InitializControlForRun();
        }

        void InitializControlForRun()
        {
            foreach (Control c in groupBox1.Controls)
            {
                CheckBox cb = c as CheckBox;
                if (cb == null) continue;

                int index = cb.Text.IndexOf(' ');
                if (index != -1)
                    cb.Text = cb.Text.Substring(0, index);
                cb.ForeColor = Color.Red;
            }
        }

        private void RunButton_Click(object sender, EventArgs e)
        {
            // validation
            errorProvider1.Clear();

            if (String.IsNullOrEmpty(BoardIdTextBox.Text))
            {
                BoardIdTextBox.Focus();
                errorProvider1.SetError(BoardIdTextBox, "没有批号");
                return;
            }

            if (string.IsNullOrEmpty(MacAddrTextBox.Text))
            {
                MacAddrTextBox.Focus();
                errorProvider1.SetError(MacAddrTextBox, "没有MAC地址");
                return;
            }

            // check mac address in format
            string mac = MacAddrTextBox.Text;
            byte[] addr = new byte[6];

            if (!TryParseMac(mac, addr))
            {
                MacAddrTextBox.Focus();
                errorProvider1.SetError(MacAddrTextBox, "MAC地址格式不对，必须是12个字符长，16进制");
                return;
            }

            String filename = String.Format("{0}.txt", MacAddrTextBox.Text);
            filename = Path.Combine(OutputPathTextBox.Text, filename);
            if (File.Exists(filename))
            {
                if (MessageBox.Show("MAC地址已经用过，你确定要这么做吗？", "错误", MessageBoxButtons.YesNo) == System.Windows.Forms.DialogResult.No)
                    return;
            }

            // check current for flashing
            float current = device.Current;

            if (current < 10 || current > 30)
            {
                if (MessageBox.Show("电流不正常，无法刷机。一定要尝试吗？", "错误", MessageBoxButtons.YesNo) == System.Windows.Forms.DialogResult.No)
                    return;
            }

            timer2.Enabled = true;
            // disable controls
            RunButton.Enabled = false;
            BoardIdTextBox.ReadOnly = true;
            MacAddrTextBox.ReadOnly = true;
            OperatorTextBox.ReadOnly = true;
            InitializControlForRun();

            foreach (Control c in groupBox1.Controls)
            {
                CheckBox cb = c as CheckBox;
                if (cb == null) continue;

                cb.Checked = false;
                cb.ForeColor = SystemColors.ControlText;
            }


            // enable status textbox
            label3.Text = "刷机";
            label3.Visible = true;


            // generate mac.txt file
            using (fs = new StreamWriter(File.OpenWrite("mac.txt")))
            {
                fs.WriteLine("@1800");
                fs.WriteLine(String.Format("{0} 00 01 01 {1:X2} {2:X2} {3:X2} {4:X2} {5:X2} {6:X2}\r\nq",
                    testingCheckbox.Checked?"01":"00",
                    addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]));
            }


            fs = new StreamWriter(File.OpenWrite(filename));

            fs.WriteLine("测试记录，MAC地址：{0} 批号：{1} 操作员：{2}", MacAddrTextBox.Text, BoardIdTextBox.Text, OperatorTextBox.Text);

            ProcessStartInfo startInfo = new ProcessStartInfo("bsl.exe");
            startInfo.Arguments = "prog";
            startInfo.CreateNoWindow = true;
            startInfo.RedirectStandardOutput = true;
            startInfo.WindowStyle = ProcessWindowStyle.Minimized;
            //startInfo.RedirectStandardError = true;
            //startInfo.RedirectStandardInput = true;
            //startInfo.WindowStyle = ProcessWindowStyle.Minimized;
            startInfo.UseShellExecute = false;

            currentstatus = Status.BurnFinish;
            timer2.Enabled = true;
            
            process = Process.Start(startInfo);
            process.EnableRaisingEvents = true;
            process.Exited += process_Exited;
            process.OutputDataReceived += process_OutputDataReceived;
            process.BeginOutputReadLine();

        }

        // return true if parse sucessfully
        private bool TryParseMac(string mac, byte[] addr)
        {
            if (mac.Length != 12)
                return false;

            for (int i = 0; i < 12; i+=2)
            {
                byte a, b;
                if (!ByteTryParse(mac[i], out a)
                    || !ByteTryParse(mac[i+1], out b))
                {
                    return false;
                }

                addr[i / 2] = (byte)(a * 16 + b);
                Debug.Write(String.Format("{0:X2} ", addr[i/2]));
            }

            return true;
        }

        private bool ByteTryParse(char p, out byte a)
        {
            if (p >= '0' && p <= '9')
            {
                a = (byte)(p - '0');
                return true;
            }

            if (p >= 'a' && p <= 'f')
            {
                a = (byte)(p - 'a' + 10);
                return true;
            }

            if (p >= 'A' && p <= 'F')
            {
                a = (byte)(p - 'A' + 10);
                return true;
            }

            a = 0;

            return false;
        }

        delegate void OutputDataReceivedCallback(object sender, DataReceivedEventArgs e);

        void process_OutputDataReceived(object sender, DataReceivedEventArgs e)
        {
            if (this.InvokeRequired)
            {
                OutputDataReceivedCallback d = new OutputDataReceivedCallback(process_OutputDataReceived);
                this.Invoke(d, new object[]{sender, e});
            }
            else
                AddNewLine(e.Data);
        }

        void process_OutputDataReceived2(object sender, DataReceivedEventArgs e)
        {
            if (this.InvokeRequired)
            {
                OutputDataReceivedCallback d = new OutputDataReceivedCallback(process_OutputDataReceived2);
                this.Invoke(d, new object[]{sender, e});
            }
            else
            {
                AddNewLine(e.Data);

                if (e.Data != null && e.Data.StartsWith("$$"))
                {
                    // this is test output data
                    // format is $$OK|FAIL compoenent-name
                    // format also $$END
                    string msg = e.Data;
                    if (msg.StartsWith("$$FAIL"))
                    {
                        resetprocess.Kill();
                        resetprocess = null;
                        OnFinished(Status.Fail);
                    }
                    else if (msg.StartsWith("$$OK "))
                    {
                        ValidateCurrent(msg);
                    }
                    else if (msg.StartsWith("$$END"))
                    {
                        resetprocess.Kill();
                        resetprocess = null;
                        // check every item is passed
                        bool sucessful = true;
                        foreach (Control c in groupBox1.Controls)
                        {
                            CheckBox cb = c as CheckBox;
                            if (cb == null) continue;

                            if (!cb.Checked)
                            {
                                sucessful = false;
                                break;
                            }
                        }
                        if (sucessful)
                            OnFinished(Status.TestFinish);
                        else
                            OnFinished(Status.Fail);
                    }
                }
            }
        }

        private void ValidateCurrent(string msg)
        {
            float current = device.Current;
            AddNewLine(String.Format("Current: {0}mA", current));
            // populate the passed item
            foreach (Control c in groupBox1.Controls)
            {
                CheckBox cb = c as CheckBox;
                if (cb == null) continue;

                if (cb.Text == msg.Substring(5))
                {
                    // fetch the current
                    object v = Settings.Default[cb.Text];
                    cb.Text += " " + current.ToString() + "mA";
                    if (v == null
                        || (float)v == 0
                        || (Math.Abs((float)v - current) < ((float)v * 0.1f))
                        )
                    {
                        cb.Checked = true;
                        cb.ForeColor = Color.Green;
                    }
                    else
                    {
                        cb.Checked = false;
                        cb.ForeColor = Color.Red;
                    }
                }
            }
        }

        delegate void ProcessExitedCallback(object sender, EventArgs e);
        void process_Exited(object sender, EventArgs e)
        {
            if (this.InvokeRequired)
            {
                ProcessExitedCallback d = new ProcessExitedCallback(process_Exited);
                this.Invoke(d, new object[] { sender, e });
            }
            else
            {
                OnFinished(currentstatus);
            }
        }

        private void AddNewLine(string line)
        {
            if (line == null)
                return;

            if (currentstatus == Status.TestFinish)
            {
                // flush to textbox and file
                fs.Write(line + "\n");
                LogTextBox.AppendText(line);
                LogTextBox.AppendText("\n");
            }
        }

        private void OnFinished(Status status)
        {
            if (status == Status.BurnFinish)
            {
                // continue next step
                currentstatus = Status.TestFinish;
                label3.Text = "测试";
                process = null;

                ValidateCurrent("$$OK FLASHING");

                ProcessStartInfo startInfo = new ProcessStartInfo("bsl.exe");
                startInfo.Arguments = "reset";
                startInfo.WindowStyle = ProcessWindowStyle.Minimized;
                startInfo.RedirectStandardOutput = true;
                //startInfo.RedirectStandardError = true;
                //startInfo.RedirectStandardInput = true;
                startInfo.UseShellExecute = false;
                startInfo.CreateNoWindow = true;
                //startInfo.WindowStyle = ProcessWindowStyle.Minimized;

                resetprocess = Process.Start(startInfo);
                resetprocess.EnableRaisingEvents = true;
                resetprocess.Exited += process_Exited;
                resetprocess.OutputDataReceived += process_OutputDataReceived2;
                resetprocess.BeginOutputReadLine();
            }
            else if (status != Status.Done)
            {
                currentstatus = Status.Done;
                if (status == Status.Fail)
                {
                    // warning 
                    ResultLabel.Text = "出错了";
                    fs.WriteLine("FAIL {0} {1} {2}", MacAddrTextBox.Text, BoardIdTextBox.Text, OperatorTextBox.Text);
                }
                else
                {
                    ResultLabel.Text = "测试通过";
                    fs.WriteLine("OK {0} {1} {2}", MacAddrTextBox.Text, BoardIdTextBox.Text, OperatorTextBox.Text);
                }
                fs.Close();
                // save the result
                timer2.Enabled = false;

                // enable the control, get ready for next
                RunButton.Enabled = true;
                MacAddrTextBox.ReadOnly = false;
                label3.Text = "测试通过，下一个";

                MacAddrTextBox.Focus();
                MacAddrTextBox.SelectAll();
                LogTextBox.Clear();
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog dialog = new FolderBrowserDialog();
            if (dialog.ShowDialog(this) == System.Windows.Forms.DialogResult.OK)
            {
                OutputPathTextBox.Text = dialog.SelectedPath;
            }
        }

        private void textBox2_TextChanged(object sender, EventArgs e)
        {
            if (MacAddrTextBox.Text.Length == 12)
            {
                RunButton_Click(sender, e);
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            using (OpenFileDialog diaglog = new OpenFileDialog())
            {
                diaglog.CheckFileExists = true;
                diaglog.Filter = "Firmware files (*.txt)|*.txt|All files (*.*)|*.*";
                if (diaglog.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                {
                    File.Copy(diaglog.FileName, ".\\watch.txt", true);
                }

            }
        }

        private void MainForm_FormClosed(object sender, FormClosedEventArgs e)
        {
            device.Stop();

            if (resetprocess != null)
                resetprocess.Kill();

            if (process != null)
                process.Kill();

            Application.Exit();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            currentLabel.Text = String.Format("{0}mA", device.Current);
        }

        private void timer2_Tick(object sender, EventArgs e)
        {
            timeout--;
            seclbl.Text = String.Format("{0} second", timeout);
            if (timeout == 0)
            {
                OnFinished(Status.Fail);
                timer2.Enabled = false;
            }
        }
    }
}
