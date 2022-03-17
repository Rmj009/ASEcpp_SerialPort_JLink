using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Toshiba2203.Totest
{
    internal class ToshibaTestCase : I_Base , Interface1
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
                this.Dispose();
            }
            catch (Exception ex)
            {
                throw new Exception("Testcase StartTotest err" + ex.ToString());
                Console.WriteLine("StartTotest err -->" +　ex.ToString());
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
        //-----------------------------------------------------------------------自定義 func
        public void InitSetting() { }
        public void Dispose()
        {
            throw new NotImplementedException();
        }
        public void IDispose()
        {
            throw new NotImplementedException();
        }
    }
}
