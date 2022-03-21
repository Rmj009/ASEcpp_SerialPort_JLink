using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Toshiba2203.Totest
{
    internal class ToshibaTestCase : ICBase , IIC
    {
        public class TestItem
        {
            private bool mIsRunEnd = false;
            private string mItemIndex = "";
            private string mBTDevName = "";
            private string mBTMac = "";
            private string mBTLicense = "";
            private string mTestResult = "";
            private bool mCanRunNext = true;
            private bool mIsChangeMacLogName = false;
            private string mNewLogBTMacName = "";

            public string BTDevName
            {
                get
                {
                    return this.mBTDevName;
                }

                set
                {
                    this.mBTDevName = value;
                }
            }

            public string BTLicense
            {
                get
                {
                    return this.mBTLicense;
                }

                set
                {
                    this.mBTLicense = value;
                }
            }

            public string BTMac
            {
                get
                {
                    return this.mBTMac;
                }

                set
                {
                    this.mBTMac = value;
                }
            }

            public bool IsRunEnd
            {
                get
                {
                    return this.mIsRunEnd;
                }

                set
                {
                    this.mIsRunEnd = value;
                }
            }

            public string ItemIndex
            {
                get
                {
                    return this.mItemIndex;
                }

                set
                {
                    this.mItemIndex = value;
                }
            }

            public string TestResult
            {
                get
                {
                    return this.mTestResult;
                }

                set
                {
                    this.mTestResult = value;
                }
            }

            public bool CanRunNext
            {
                get
                {
                    return this.mCanRunNext;
                }

                set
                {
                    this.mCanRunNext = value;
                }
            }

            public bool IsChangeMacLogName
            {
                get
                {
                    return this.mIsChangeMacLogName;
                }

                set
                {
                    this.mIsChangeMacLogName = value;
                }
            }

            public string NewLogBTMacName
            {
                get
                {
                    return this.mNewLogBTMacName;
                }

                set
                {
                    this.mNewLogBTMacName = value;
                }
            }

            public string License { get; set; }
        }

        public void StartToTest(object testItem) 
        {
            TestItem testItem1 = (TestItem)testItem;

            try
            {
                this.WriteProcessFlag();
                this.ReadProcessFlag();
                this.SetDcDc();
                this.EndDTM();
                this.ReadBDAdress();
                this.ReadFwVer();
                this.SetSleepMode(false);
                this.CheckXtalFreqQ(false);
                this.SetDcDc();
                //this.Dispose();
            }
            catch (Exception ex)
            {
                Console.WriteLine("StartTotest err -->" +　ex.ToString());
                throw new Exception("Testcase StartTotest err" + ex.ToString());
            }

        }

        /// <summary>
        /// Toshiba Define TestItems as follows
        /// </summary>
        public void WriteProcessFlag() { }
        public void ReadProcessFlag() { }
        public void SetDTM() { }
        public void EndDTM() { }
        public void ReadBDAdress() { }
        public void ReadFwVer() { }
        public void SetSleepMode(bool flag) { }
        //public bool IsSleepMode() { return false; }
        public void CheckXtalFreqQ(bool flag) { }
        public void SetDcDc() { }
        //-----------------------------------------------------------------------clone 自定義 func
        public void InitSetting() { }
        public void Dispose()
        {
            try
            {
                //this.PowerOffDUTNoLog();
                //this.SafeCloseInstrument(mRFInstrument);
                //this.SafeCloseInstrument(mExtraAdapPowerInstrument);
                //this.SafeCloseInstrument(mExtraVBatPowerInstrument);
                //this.SafeCloseInstrument(mDaqInstrument);
                //this.SafeCloseInstrument(mGPIODaqInstrument);
            }
            catch { }
        }

        public void IDispose()
        {
            throw new NotImplementedException();
            //Serail_DLL.Multi_SerialCloseAll();
            //this.ClosePixartPGM();
            //this.SafeDisposeInstrument(this.mRFInstrument);
            //this.SafeDisposeInstrument(mExtraAdapPowerInstrument);
            //this.SafeDisposeInstrument(mExtraVBatPowerInstrument);
            //this.SafeDisposeInstrument(mDaqInstrument);
            //this.SafeDisposeInstrument(mGPIODaqInstrument);
        }

        private void SendCmd(string cmd, ref string result, bool showLogForUI = true)
        {
            if (showLogForUI)
            {
                if (SystemIni.Instance.Enable_Verbose())
                {
                    this.Save_LOG_data("Send command : " + cmd);
                }
            }
            result = "";
            Serail_DLL.SERIAL_RESULT serialResult = new Serail_DLL.SERIAL_RESULT();
            // Console.WriteLine("Send Cmd : " + cmd);
            string comport = this.GetnRFSerialPort();

            if (!Serail_DLL.Multi_TransferReceive_HEX(comport, cmd.Replace(" ", ""), ref serialResult, SERIAL_TIME_OUT))
            {
                throw new Exception("Serial Timeout Error !! Command : " + cmd);
            }

            string sResult = serialResult.readData.Trim();
            if (showLogForUI)
            {
                if (SystemIni.Instance.Enable_Verbose())
                {
                    this.Save_LOG_data("Receice Command ：" + sResult);
                }
            }

            result = sResult;
        }

        private string GetnRFSerialPort()
        {
            string port = "";
            try
            {
                using (ManagementObjectSearcher searcher = new ManagementObjectSearcher("select * from Win32_PnPEntity"))
                {

                    foreach (var hardInfo in searcher.Get())
                    {
                        if (hardInfo.Properties["Name"].Value != null && hardInfo.Properties["Name"].Value.ToString().Contains("(COM") && hardInfo.ToString().Contains("FTDIBUS"))
                        {
                            string[] sArrayPatch = hardInfo.Properties["Name"].Value.ToString().Split('(');

                            port = sArrayPatch[1].Trim().ToString().Replace(")", "");

                        }
                    }
                }
            }
            catch
            {
                throw;
            }

            return port;
            //return "COM3";
        }

        private void FWVerCheck()
        {
            string sResult = "";
            string title = "Start FW Ver Check";

            try
            {
                Console.WriteLine(title);
                SendCmd("0x01 06", ref sResult, true);
            }
            catch (Exception ex)
            {

                throw ex;
            }
        }
        private void ProcessFlagWritingInitialize()
        {
            string sResult = "";
            string title = "Process flag writing (Initialize)";

            try
            {
                Console.WriteLine(title);
                SendCmd("0x03 01 00 5A", ref sResult, true);
                SendCmd("0x03 01 01 5A", ref sResult, true);
                SendCmd("0x03 01 02 5A", ref sResult, true);
                SendCmd("0x03 01 03 5A", ref sResult, true);
            }
            catch (Exception ex)
            {
                throw ex;
            }

        }
        private void ProcessFlagWritingOK()
        {
            string sResult = "";
            string title = "===Process flag writing (Write OK flag)===";

            try
            {
                Console.WriteLine(title);
                SendCmd("0x03 01 00 00", ref sResult, true);
            }
            catch (Exception ex)
            {

                throw ex;
            }
        }
        private void ProcessFlagReading()
        {
            string sResult = "";
            string title = "===Process flag reading===";

            try
            {
                Console.WriteLine(title);
                SendCmd("0x02 02 FF", ref sResult, true);
            }
            catch (Exception ex)
            {
                throw ex;
            }
        }
        private void FWeraseALL()
        {
            string sResult = "";
            string title = "===FW erase（All Clear）===";
            //string TestCondition = "Test FW erase＠Use "J - Flash"";

            try
            {
                Console.WriteLine(title);
            }
            catch (Exception ex)
            {

                throw ex;
            }
        }


        //public void SetICSystemIni(object iniObject)
        //{
        //    this.mSystemConfig = iniObject as SASP8SystemIni;
        //}

        //public void SetLogCallback(Action<string, uint> logAction)
        //{
        //    this.mLogCallback = logAction;
        //}

        //public void SetRunResultCallback(Action<string, bool> resultCallback)
        //{
        //    this.mRunResultCallback = resultCallback;
        //}

        //public void SetUICallback(Action<string> uiCallback)
        //{
        //    this.mUICallback = uiCallback;
        //}

        //public void Save_LOG_data(string sTtestResult, bool isTitle = false, bool isCustom = false, bool isError = false)
        //{
        //    uint type = isTitle ? RFTestTool.Util.MSG.TITLE : RFTestTool.Util.MSG.NORMAL;
        //    if (type == RFTestTool.Util.MSG.TITLE)
        //    {
        //        sTtestResult = "*** " + sTtestResult + " ***";
        //    }
        //    type = isCustom ? RFTestTool.Util.MSG.CUSTOM : type;
        //    type = isError ? RFTestTool.Util.MSG.ERROR : type;

        //    this.mLogCallback(sTtestResult, type);
        //}



    }
}
