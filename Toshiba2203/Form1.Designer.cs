namespace Toshiba2203
{
    partial class Form1
    {
        /// <summary>
        ///  Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        ///  Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        ///  Required method for Designer support - do not modify
        ///  the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.TryToLoad_Btn = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // TryToLoad_Btn
            // 
            this.TryToLoad_Btn.Location = new System.Drawing.Point(12, 195);
            this.TryToLoad_Btn.Name = "TryToLoad_Btn";
            this.TryToLoad_Btn.Size = new System.Drawing.Size(94, 29);
            this.TryToLoad_Btn.TabIndex = 0;
            this.TryToLoad_Btn.Text = "Load";
            this.TryToLoad_Btn.UseVisualStyleBackColor = true;
            this.TryToLoad_Btn.Click += new System.EventHandler(this.TryToLoad_Btn_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(9F, 19F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 450);
            this.Controls.Add(this.TryToLoad_Btn);
            this.Name = "Form1";
            this.Text = "Form1";
            this.ResumeLayout(false);

        }

        #endregion

        private Button TryToLoad_Btn;
    }
}