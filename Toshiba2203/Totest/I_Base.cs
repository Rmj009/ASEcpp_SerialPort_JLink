using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Toshiba2203.Totest;

namespace Toshiba2203.Totest
{
    public abstract class I_Base
    {
        private bool mIsTesting = false;
        private System.Threading.Mutex mTestingMutex = new System.Threading.Mutex();
        public bool IsTesting()
        {
            bool isTesting = false;
            mTestingMutex.WaitOne();
            isTesting = this.mIsTesting;
            mTestingMutex.ReleaseMutex();
            return isTesting;
        }

        public void SetIsTesting(bool isTesting)
        {
            mTestingMutex.WaitOne();
            this.mIsTesting = isTesting;
            mTestingMutex.ReleaseMutex();
        }
    }
}
