using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Toshiba2203.Totest
{
    internal interface IIC         //SAPA8 toshiba only
    {
        bool IsTesting();
        void SetIsTesting(bool isTesting);
        void StartToTest(object testItem);
        void Dispose();
        void IDispose();
    }
}
