using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Toshiba2203.Totest
{
    internal interface Interface1
    {
        bool IsTesting();
        void SetIsTesting(bool isTesting);
        void StartToTest(object testItem);
        void Dispose();
        void IDispose();
    }
}
