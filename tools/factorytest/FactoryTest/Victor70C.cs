using System;
using System.Text;
using System.Threading;
using USBHIDDRIVER;


namespace FactoryTest
{
    public class Victor70C
    {
        const byte VICTOR70C_DIGIT0_0 = 0x81;
        const byte VICTOR70C_DIGIT0_1 = 0x01;
        const byte VICTOR70C_DIGIT0_2 = 0xC1;
        const byte VICTOR70C_DIGIT0_3 = 0x41;
        const byte VICTOR70C_DIGIT0_4 = 0xA1;
        const byte VICTOR70C_DIGIT0_5 = 0x21;
        const byte VICTOR70C_DIGIT0_6 = 0xE1;
        const byte VICTOR70C_DIGIT0_7 = 0x61;
        const byte VICTOR70C_DIGIT0_8 = 0x91;
        const byte VICTOR70C_DIGIT0_9 = 0x11;

        const byte VICTOR70C_DIGIT1_0 = 0x6F;
        const byte VICTOR70C_DIGIT1_1 = 0xEF;
        const byte VICTOR70C_DIGIT1_2 = 0xAF;
        const byte VICTOR70C_DIGIT1_3 = 0x2F;
        const byte VICTOR70C_DIGIT1_4 = 0x8F;
        const byte VICTOR70C_DIGIT1_5 = 0x0F;
        const byte VICTOR70C_DIGIT1_6 = 0xCF;
        const byte VICTOR70C_DIGIT1_7 = 0x4F;
        const byte VICTOR70C_DIGIT1_8 = 0x7F;
        const byte VICTOR70C_DIGIT1_9 = 0xFF;

        const byte VICTOR70C_DIGIT2_0 = 0x71;
        const byte VICTOR70C_DIGIT2_1 = 0xF1;
        const byte VICTOR70C_DIGIT2_2 = 0xB1;
        const byte VICTOR70C_DIGIT2_3 = 0x31;
        const byte VICTOR70C_DIGIT2_4 = 0x91;
        const byte VICTOR70C_DIGIT2_5 = 0x11;
        const byte VICTOR70C_DIGIT2_6 = 0xD1;
        const byte VICTOR70C_DIGIT2_7 = 0x51;
        const byte VICTOR70C_DIGIT2_8 = 0x81;
        const byte VICTOR70C_DIGIT2_9 = 0x01;

        const byte VICTOR70C_DIGIT3_0 = 0x77;
        const byte VICTOR70C_DIGIT3_1 = 0xF7;
        const byte VICTOR70C_DIGIT3_2 = 0xB7;
        const byte VICTOR70C_DIGIT3_3 = 0x37;
        const byte VICTOR70C_DIGIT3_4 = 0x97;
        const byte VICTOR70C_DIGIT3_5 = 0x17;
        const byte VICTOR70C_DIGIT3_6 = 0xD7;
        const byte VICTOR70C_DIGIT3_7 = 0x57;
        const byte VICTOR70C_DIGIT3_8 = 0x87;
        const byte VICTOR70C_DIGIT3_9 = 0x07;

        float current;
        USBInterface device;

        public float Current
        {
            get
            {
                return current;
            }
        }

        static float Decode(byte[] buf)
        {
            int[] digit = new int[4] { 0, 0, 0, 0 };
            switch (buf[7])
            {
                case VICTOR70C_DIGIT0_0:
                    digit[0] = 0;
                    break;
                case VICTOR70C_DIGIT0_1:
                    digit[0] = 1;
                    break;
                case VICTOR70C_DIGIT0_2:
                    digit[0] = 2;
                    break;
                case VICTOR70C_DIGIT0_3:
                    digit[0] = 3;
                    break;
                case VICTOR70C_DIGIT0_4:
                    digit[0] = 4;
                    break;
                case VICTOR70C_DIGIT0_5:
                    digit[0] = 5;
                    break;
                case VICTOR70C_DIGIT0_6:
                    digit[0] = 6;
                    break;
                case VICTOR70C_DIGIT0_7:
                    digit[0] = 7;
                    break;
                case VICTOR70C_DIGIT0_8:
                    digit[0] = 8;
                    break;
                case VICTOR70C_DIGIT0_9:
                    digit[0] = 9;
                    break;
                default:
                    Console.WriteLine("Invalid digit0 = {0}", buf[7]);
                    break;
            }


            switch (buf[10])
            {
                case VICTOR70C_DIGIT1_0:
                    digit[1] = 0;
                    break;
                case VICTOR70C_DIGIT1_1:
                    digit[1] = 1;
                    break;
                case VICTOR70C_DIGIT1_2:
                    digit[1] = 2;
                    break;
                case VICTOR70C_DIGIT1_3:
                    digit[1] = 3;
                    break;
                case VICTOR70C_DIGIT1_4:
                    digit[1] = 4;
                    break;
                case VICTOR70C_DIGIT1_5:
                    digit[1] = 5;
                    break;
                case VICTOR70C_DIGIT1_6:
                    digit[1] = 6;
                    break;
                case VICTOR70C_DIGIT1_7:
                    digit[1] = 7;
                    break;
                case VICTOR70C_DIGIT1_8:
                    digit[1] = 8;
                    break;
                case VICTOR70C_DIGIT1_9:
                    digit[1] = 9;
                    break;
                default:
                    Console.WriteLine("Invalid digit1 = {0}", buf[10]);
                    break;
            }



            switch (buf[4])
            {
                case VICTOR70C_DIGIT2_0:
                    digit[2] = 0;
                    break;
                case VICTOR70C_DIGIT2_1:
                    digit[2] = 1;
                    break;
                case VICTOR70C_DIGIT2_2:
                    digit[2] = 2;
                    break;
                case VICTOR70C_DIGIT2_3:
                    digit[2] = 3;
                    break;
                case VICTOR70C_DIGIT2_4:
                    digit[2] = 4;
                    break;
                case VICTOR70C_DIGIT2_5:
                    digit[2] = 5;
                    break;
                case VICTOR70C_DIGIT2_6:
                    digit[2] = 6;
                    break;
                case VICTOR70C_DIGIT2_7:
                    digit[2] = 7;
                    break;
                case VICTOR70C_DIGIT2_8:
                    digit[2] = 8;
                    break;
                case VICTOR70C_DIGIT2_9:
                    digit[2] = 9;
                    break;
                default:
                    Console.WriteLine("Invalid digit2 = {0}", buf[4]);
                    break;
            }

            switch (buf[11])
            {
                case VICTOR70C_DIGIT3_0:
                    digit[3] = 0;
                    break;
                case VICTOR70C_DIGIT3_1:
                    digit[3] = 1;
                    break;
                case VICTOR70C_DIGIT3_2:
                    digit[3] = 2;
                    break;
                case VICTOR70C_DIGIT3_3:
                    digit[3] = 3;
                    break;
                case VICTOR70C_DIGIT3_4:
                    digit[3] = 4;
                    break;
                case VICTOR70C_DIGIT3_5:
                    digit[3] = 5;
                    break;
                case VICTOR70C_DIGIT3_6:
                    digit[3] = 6;
                    break;
                case VICTOR70C_DIGIT3_7:
                    digit[3] = 7;
                    break;
                case VICTOR70C_DIGIT3_8:
                    digit[3] = 8;
                    break;
                case VICTOR70C_DIGIT3_9:
                    digit[3] = 9;
                    break;
                default:
                    Console.WriteLine("Invalid digit3 = {0}", buf[11]);
                    break;
            }

            int dec = (buf[6] & 0xE0) >> 5;
            bool minus = (buf[5] & 0x01) != 0;
            bool milli = (buf[13] & 0x02) != 0;

            float ret = 0;
            if (dec == 0x00)
            {
                ret = digit[3] + digit[2] * 0.1f + digit[1] * 0.01f + digit[0] * 0.001f;
            }
            else if (dec == 0x06)
            {
                ret = digit[3] * 10 + digit[2] + digit[1] * 0.1f + digit[0] * 0.01f;
            }
            else if (dec == 0x05)
            {
                ret = digit[3] * 100 + digit[2] * 10 + digit[1] + digit[0] * 0.1f;
            }

            if (minus)
                return -ret;
            else
                return ret;
        }

        /// <summary>
        /// The event cacher.
        /// </summary>
        /// <param name="sender">The sender.</param>
        /// <param name="e">The <see cref="System.EventArgs"/> instance containing the event data.</param>
        public void myEventCacher(object sender, System.EventArgs e)
        {
            while (USBHIDDRIVER.USBInterface.usbBuffer.Count > 0)
            {
                byte[] currentRecord = null;

                lock (USBHIDDRIVER.USBInterface.usbBuffer.SyncRoot)
                {
                    currentRecord = USBHIDDRIVER.USBInterface.usbBuffer.First.Value;
                    USBHIDDRIVER.USBInterface.usbBuffer.RemoveFirst();
                }

                if (currentRecord[1] == 0)
                    continue;

                current = Decode(currentRecord);
            }
        }

        public void Start()
        {
            device = new USBInterface("vid_1244", "pid_d237");

            if (!device.Connect())
                throw new Exception("Victor 70C Multimeter cannot be found");

            device.enableUsbBufferEvent(new System.EventHandler(myEventCacher));

            Thread.Sleep(5);
            device.startRead();
        }

        public void Stop()
        {
            device.stopRead();
            device.Disconnect();
        }
    }
}
