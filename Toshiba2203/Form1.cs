using ProjToshiba.LogSingleton;
using System.Text;

namespace Toshiba2203
{
    public partial class Form1 : Form
    {
        private static LogSingleton mSingleton = null;
        StringBuilder str = new StringBuilder();

        internal static LogSingleton MSingleton { get => mSingleton; set => mSingleton = value; }

        public Form1()
        {
            InitializeComponent();
        }

        private void TryToLoad_Btn_Click(object sender, EventArgs e)
        {
            MSingleton = new LogSingleton();
            MSingleton.WriteLog(str.ToString());
        }

        private void Show_Error(string errorMsg)
        {
            MSingleton.WriteLog(errorMsg, LogSingleton.ERROR);
            MessageBox.Show(errorMsg, @"Oops!! An Error Occurred !!!", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        public void LogLinklst(string len, string cmdID, int reqData) 
        {
            //Int32 len = 0;
            //Int32 cmdID = 0;
            //Int32 reqData = 0;
            List<string> lst = new List<string>();
            lst.Concat(lst.Select(x => x.ToString()));
            LogSingleton.LinkedLst list1 = new LogSingleton.LinkedLst();
            LogSingleton.LinkedLst list2 = new LogSingleton.LinkedLst();
            LogSingleton.Node node1 = new LogSingleton.Node(len,cmdID, reqData);
            //list1.DoInsert(node1); ?????
            lst.Add(len);
            lst.Add(cmdID);
            lst.Add(reqData.ToString());
            string RequestCmmd = String.Join("U+002C", lst.ToArray());
            
        }

        private void ShowTestGroupInfo(bool isFinishThisTime, bool isError = false) 
        {
            StringBuilder sb = new StringBuilder();
            if (isError)
            {
                //this.TestInforGroup.Text = "Test Fail";
                Show_Error("Test Fail !!!!");
            }
            else
            {
                //this.TestInforGroup.Text = isFinishThisTime ? @"Finish" : @"Testing..";
                if (isFinishThisTime)
                {
                    MessageBox.Show(@"Done", @"Finish", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
            }
            MSingleton.WriteLog(str.ToString() + "\r\n------------------------------------------- ");
            sb.Clear();

            if (isFinishThisTime && isError)
            {
                str.Append("\r\n");
                str.Append("\n  ***************     ************     ****************   **              ");
                str.Append("\n  ***************   ****************   ****************   **              ");
                str.Append("\n  **                **            **         ****         **              ");
                str.Append("\n  **                **            **         ****         **              ");
                str.Append("\n  ************      ****************         ****         **              ");
                str.Append("\n  ************      ****************         ****         **              ");
                str.Append("\n  **                **            **         ****         **              ");
                str.Append("\n  **                **            **         ****         **              ");
                str.Append("\n  **                **            **         ****         **              ");
                str.Append("\n  **                **            **   ****************   ****************");
                str.Append("\n  **                **            **   ****************   ****************");
                str.Append("\r\n");
                //str.Append("\n==>  FAIL" + ((BaseException)ex).Code);
                //str.Append("\r\n" + ex.ToString());
                MSingleton.WriteLog(str.ToString());
            }
            else if (isFinishThisTime && !isError)
            {
                str.Append("\r\n");
                str.Append("\n  **********                **                 ******                ******          ");
                str.Append("\n  ************            ******             ***********           ***********       ");
                str.Append("\n  **          **         ********          ****        ****      ****        ****    ");
                str.Append("\n  **          **        **      **             ****                  ****            ");
                str.Append("\n  ************         **        **               ****                  ****         ");
                str.Append("\n  ***********         **************	             ****	              ****	  ");
                str.Append("\n  **                 ****************                  ****                  ****    ");
                str.Append("\n  **                **              **      ****         ****     ****         ****  ");
                str.Append("\n  **               **                **        ************          ************    ");
                str.Append("\n  **              **                  **          ******                ******       ");
                str.Append("\r\n");
                //str.Append("\n!!!!!! PASS !!!!!!\r\n");
                MSingleton.WriteLog(str.ToString());
            }

        }
    }
}