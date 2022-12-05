using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ASECL_JLINK_CSharp
{
    public partial class Form1 : Form
    {
        internal class JLINK_DLL
        {
            private const string DLL_PATH = "ASECL_JLink.dll";

            public delegate void LogMsg([MarshalAsAttribute(UnmanagedType.LPWStr)] string sErr);

            public delegate void BurnProcess(int percent);

            [DataContract]
            [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi, Pack = 1), Serializable]
            public struct UICR_READ_DATA
            {
                [DataMember]
                [MarshalAs(UnmanagedType.ByValArray, SizeConst = 12)]
                public byte[] datas;
            };

            [DllImport(DLL_PATH, SetLastError = true, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
            public static extern void SetLogCallback(IntPtr msg);

            [DllImport(DLL_PATH, SetLastError = true, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
            public static extern bool BurnNordic52840FWBySWD(string fwPos, ulong flashOffset = 0x00);

            [DllImport(DLL_PATH, SetLastError = true, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
            public static extern bool EraseNordic52840BySWD();

            [DllImport(DLL_PATH, SetLastError = true, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
            public static extern bool ErasePageNordic52840BySWD(UInt32 pageAddr);

            [DllImport(DLL_PATH, SetLastError = true, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
            public static extern bool ResetNordic52840BySWD();

            [DllImport(DLL_PATH, SetLastError = true, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
            public static extern bool InitJLinkModule();

            [DllImport(DLL_PATH, SetLastError = true, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
            public static extern bool FreeJLinkModule();

            [DllImport(DLL_PATH, SetLastError = true, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
            public static extern IntPtr GetLastErrorStr();

            [DllImport(DLL_PATH, SetLastError = true, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
            public static extern IntPtr GetJLinkInfo(ref bool result);

            [DllImport(DLL_PATH, SetLastError = true, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
            public static extern bool EraseNordic52840BySWDRegister();

            [DllImport(DLL_PATH, SetLastError = true, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
            public static extern bool WriteDataToUICR_Nordic52840BySWDRegister(byte[] datas, UInt32 len, UInt32 UICRStartPos = 0x80);

            [DllImport(DLL_PATH, SetLastError = true, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
            public static extern bool ReadDataFromUICR_Nordic52840BySWDRegister(ref UICR_READ_DATA outputData, UInt32 UICRStartPos = 0x80);

            [DllImport(DLL_PATH, SetLastError = true, CharSet = CharSet.Unicode, CallingConvention = CallingConvention.Cdecl)]
            public static extern bool ReadFICR_DeviceID_Nordic52840BySWDRegister(ref UInt64 outputData);
        }

        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            JLINK_DLL.LogMsg mLogFunc = (string mylog) =>
            {
                Console.WriteLine(mylog);
            };
            IntPtr intptr_delegate = Marshal.GetFunctionPointerForDelegate(mLogFunc);
            JLINK_DLL.SetLogCallback(intptr_delegate);
            JLINK_DLL.InitJLinkModule();

            //string jlinkPath = System.Windows.Forms.Application.StartupPath + @"\JLINK\JLink.exe";
            //JLINK_DLL.SetJlinkExePos(jlinkPath);
        }

        private void BurnBtn_Click(object sender, EventArgs e)
        {
            string fwPath = System.Windows.Forms.Application.StartupPath + @"\SASP_TEST_FW_pca10040_s132_v1.0.hex";
            if (!JLINK_DLL.BurnNordic52840FWBySWD(fwPath))
            {
                Console.WriteLine("Error");
            }
            else
            {
                Console.WriteLine("OK");
            }
        }

        private void EraseBtn_Click(object sender, EventArgs e)
        {
            if (!JLINK_DLL.EraseNordic52840BySWD())
            {
                Console.WriteLine("Error");
            }
            else
            {
                Console.WriteLine("OK");
            }
        }

        private void ResetBtn_Click(object sender, EventArgs e)
        {
            if (!JLINK_DLL.ResetNordic52840BySWD())
            {
                Console.WriteLine("Error");
            }
            else
            {
                Console.WriteLine("OK");
            }
        }

        private void _ShowJLinkInfo()
        {
            bool result = false;
            IntPtr info = JLINK_DLL.GetJLinkInfo(ref result);

            if (info == IntPtr.Zero || !result)
            {
                IntPtr error = JLINK_DLL.GetLastErrorStr();
                if (error != IntPtr.Zero)
                {
                    string errorStr = Marshal.PtrToStringUni(error);
                    MessageBox.Show(this, errorStr);
                }
                return;
            }

            string infoStr = Marshal.PtrToStringUni(info);
            Console.WriteLine(infoStr);
            MessageBox.Show(this, infoStr);
        }

        private void InfoBtn_Click(object sender, EventArgs e)
        {
            this._ShowJLinkInfo();
        }

        private void _ShowLastError()
        {
            IntPtr error = JLINK_DLL.GetLastErrorStr();
            if (error != IntPtr.Zero)
            {
                string errorStr = Marshal.PtrToStringUni(error);
                if (errorStr.Trim().Length == 0)
                {
                    MessageBox.Show(this, "No Error !!");
                }
                else
                {
                    MessageBox.Show(this, errorStr);
                }
            }
            else
            {
                MessageBox.Show(this, "No Error !!");
            }
        }

        private void ErrorBtn_Click(object sender, EventArgs e)
        {
            this._ShowLastError();
        }

        private void ErasePartitionBtn_Click(object sender, EventArgs e)
        {
            uint addr = 0x00a0;
            if (!JLINK_DLL.ErasePageNordic52840BySWD(addr))
            {
                Console.WriteLine("Error");
            }
            else
            {
                Console.WriteLine("OK");
            }
        }

        private void EraseByRegisterBtn_Click(object sender, EventArgs e)
        {
            if (!JLINK_DLL.EraseNordic52840BySWDRegister())
            {
                Console.WriteLine("Error");
            }
            else
            {
                Console.WriteLine("OK");
            }
        }

        private void WriteUICRBtn_Click(object sender, EventArgs e)
        {
            string ss = "220811041411";
            byte[] bytes = Encoding.ASCII.GetBytes(ss);
            if (!JLINK_DLL.WriteDataToUICR_Nordic52840BySWDRegister(bytes, 12))
            {
                Console.WriteLine("Error");
            }
            else
            {
                Console.WriteLine("OK");
            }

            JLINK_DLL.UICR_READ_DATA data = new JLINK_DLL.UICR_READ_DATA();
            if (!JLINK_DLL.ReadDataFromUICR_Nordic52840BySWDRegister(ref data))
            {
                Console.WriteLine("Error");
            }
            else
            {
                Console.WriteLine("OK");
                string str = Encoding.ASCII.GetString(data.datas);
                Console.WriteLine("Src --> " + ss);
                Console.WriteLine("Result --> " + str);
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            UInt64 result = 0;
            if (!JLINK_DLL.ReadFICR_DeviceID_Nordic52840BySWDRegister(ref result))
            {
                Console.WriteLine("Error");
            }
            else
            {
                Console.WriteLine("Result --> " + result.ToString());
                Console.WriteLine("OK");
            }
        }
    }
}