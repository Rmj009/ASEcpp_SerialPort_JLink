using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Toshiba2203.Exception
{
    class BaseException : System.Exception
    {
        private string mCode = "-1000";
        private string mPrefix = "";
        protected string Title = "BaseException";

        public string Code
        {
            get
            {
                return this.mCode;
            }

            set
            {
                this.mCode = value;
            }
        }

        public string Prefix
        {
            get
            {
                return this.mPrefix;
            }

            set
            {
                this.mPrefix = value;
            }
        }

        public BaseException(string message, string code) : base(message)
        {
            this.mCode = code;
        }

        public override string Message
        {
            get
            {
                if (this.Code.Equals("-1000"))
                {
                    return base.Message;
                }
                else
                {
                    if (this.Prefix.Length == 0)
                    {
                        return string.Format("Error Code:{0}, {1} : Message:{2}", this.Code, this.Title, base.Message);
                    }
                    else
                    {
                        return string.Format("{0} Error Code:{1}, {2} : Message:{3}", this.Prefix, this.Code, this.Title, base.Message);
                    }

                }
            }
        }
    }
}
