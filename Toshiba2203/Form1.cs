using ProjToshiba.LogSingleton;
using System.Text;

namespace Toshiba2203
{
    public partial class Form1 : Form
    {
        public static LogSingleton mSingleton = null;
        StringBuilder str = new StringBuilder();

        public Form1()
        {
            InitializeComponent();
        }

        private void TryToLoad_Btn_Click(object sender, EventArgs e)
        {
            mSingleton = new LogSingleton();
            mSingleton.WriteLog(str.ToString());
        }
    }
}