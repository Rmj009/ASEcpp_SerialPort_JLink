using System.Text;
using System.Text.Encodings;

namespace ProjToshiba.LogSingleton
{
    internal class LogSingleton
    {

        //private static volatile LogSingleton m_singletonLog = null;
        ////  private static object m_threadLock = new object();
        //private static System.Threading.Mutex ms_threadMutex = new System.Threading.Mutex();
        public static int NORMAL = 0;
        public static int ERROR = 1;
        public static int SCPI = 2;
        public static int TOSHIBA_SEND = 3;
        public static int TOSHIBA_RECEIVE = 4;

        private static readonly object mLock = new object();
        //private static LogSingleton mInstance = null;
        private static System.Threading.Mutex ms_threadMutex = new System.Threading.Mutex();

        private const int mc_logMaxCount = 30;
        //private string m_logDirPath = System.Windows.Forms.Application.StartupPath + @"\Log\" + DateTime.Now.ToString("yyyyMMdd");
        private static string temp_logFileName = "";
        //private static string m_logFileName = "";
        //private static string TMP_FILE = "tmp.log";
        //private string mRootFlowPath = "";
        //private string mRootPath = "";

        /// <summary>
        /// 採用 singleton 模式
        /// </summary>
        /// <returns></returns>
        /// 
        public class Node 
        {
            public string cmdlen; //data
            public string codenames;
            public int data;
            public Node next;
            //  1. Len 2.Command ID 3.Data
            public Node(string cmdlen, string codenames, int data) { this.cmdlen = cmdlen; this.data = data; this.codenames = codenames; this.next = null; }

        }
       
        public class LinkedLst 
        {
            private Node first;
            private Node last;
            List<Node> lst;
            
        public bool isEmpty() 
            {
                return this.first != null && this.last != null;
            }
            public void Print() 
            {
                Node current = this.first;
                while (current!=null)
                {
                    Console.WriteLine("[" + current.cmdlen + " " + current.codenames + " " + current.data + "]");
                }
                Console.WriteLine();
            }
            public void DoInsert(string cmdlen, string codenames, int data) 
            {
                Node newNode = new Node(cmdlen, codenames, data);
                if (this.isEmpty())
                {
                    first = newNode;
                    last = newNode;
                }
                else
                {
                    last.next = newNode;
                    last = newNode;
                }
            }
        }


        public void WriteLog(string content, int type = 0)
        {
            ms_threadMutex.WaitOne();
            if (temp_logFileName.Length == 0)
            {
                ms_threadMutex.ReleaseMutex();
                return;
            }

            string nowDateTime = "";
            if (type == LogSingleton.NORMAL)
            {
                nowDateTime = @"[ " + DateTime.Now.ToString(@"yyyy/MM/dd HH:mm:ss") + @" ] : ";
            }
            else if (type == LogSingleton.SCPI)
            {
                nowDateTime = @"[ " + DateTime.Now.ToString(@"yyyy/MM/dd HH:mm:ss") + @"_ SCPI ] : ";
            }
            else if (type == LogSingleton.TOSHIBA_SEND)
            {
                nowDateTime = @"[ " + DateTime.Now.ToString(@"Send command") + @" ] : ";
            }
            else if (type == LogSingleton.TOSHIBA_RECEIVE)
            {
                nowDateTime = @"[ " + DateTime.Now.ToString(@"Receive command") + @" ] : ";
            }
            else
            {
                nowDateTime = @"[ " + DateTime.Now.ToString(@"yyyy/MM/dd HH:mm:ss") + @"_ 錯誤 ] : ";
            }
            //string nowDateTime = @"[ " + DateTime.Now.ToString(@"yyyy/MM/dd HH:mm:ss") + @" _ 系統訊息 ] : ";
            // string nowDateTime = @"[ " + DateTime.Now.ToString(@"yyyy/MM/dd HH:mm:ss") + @" ] : ";
            System.IO.FileStream fStream = new System.IO.FileStream(temp_logFileName, System.IO.FileMode.Append, System.IO.FileAccess.Write);
            System.IO.StreamWriter sWriter = new System.IO.StreamWriter(fStream, Encoding.Unicode);
            sWriter.WriteLine(nowDateTime + content);
            sWriter.Close();
            fStream.Close();
            ms_threadMutex.ReleaseMutex();
        }

    }
}
