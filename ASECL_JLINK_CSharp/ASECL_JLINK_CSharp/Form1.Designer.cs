
namespace ASECL_JLINK_CSharp
{
    partial class Form1
    {
        /// <summary>
        /// 設計工具所需的變數。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清除任何使用中的資源。
        /// </summary>
        /// <param name="disposing">如果應該處置受控資源則為 true，否則為 false。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form 設計工具產生的程式碼

        /// <summary>
        /// 此為設計工具支援所需的方法 - 請勿使用程式碼編輯器修改
        /// 這個方法的內容。
        /// </summary>
        private void InitializeComponent()
        {
            this.BurnBtn = new System.Windows.Forms.Button();
            this.EraseBtn = new System.Windows.Forms.Button();
            this.ResetBtn = new System.Windows.Forms.Button();
            this.ErrorBtn = new System.Windows.Forms.Button();
            this.InfoBtn = new System.Windows.Forms.Button();
            this.ErasePartitionBtn = new System.Windows.Forms.Button();
            this.EraseByRegisterBtn = new System.Windows.Forms.Button();
            this.WriteUICRBtn = new System.Windows.Forms.Button();
            this.button1 = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // BurnBtn
            // 
            this.BurnBtn.Location = new System.Drawing.Point(27, 45);
            this.BurnBtn.Name = "BurnBtn";
            this.BurnBtn.Size = new System.Drawing.Size(205, 90);
            this.BurnBtn.TabIndex = 0;
            this.BurnBtn.Text = "Burn";
            this.BurnBtn.UseVisualStyleBackColor = true;
            this.BurnBtn.Click += new System.EventHandler(this.BurnBtn_Click);
            // 
            // EraseBtn
            // 
            this.EraseBtn.Location = new System.Drawing.Point(287, 45);
            this.EraseBtn.Name = "EraseBtn";
            this.EraseBtn.Size = new System.Drawing.Size(205, 90);
            this.EraseBtn.TabIndex = 1;
            this.EraseBtn.Text = "Erase";
            this.EraseBtn.UseVisualStyleBackColor = true;
            this.EraseBtn.Click += new System.EventHandler(this.EraseBtn_Click);
            // 
            // ResetBtn
            // 
            this.ResetBtn.Location = new System.Drawing.Point(538, 45);
            this.ResetBtn.Name = "ResetBtn";
            this.ResetBtn.Size = new System.Drawing.Size(205, 90);
            this.ResetBtn.TabIndex = 2;
            this.ResetBtn.Text = "Reset";
            this.ResetBtn.UseVisualStyleBackColor = true;
            this.ResetBtn.Click += new System.EventHandler(this.ResetBtn_Click);
            // 
            // ErrorBtn
            // 
            this.ErrorBtn.Location = new System.Drawing.Point(27, 198);
            this.ErrorBtn.Name = "ErrorBtn";
            this.ErrorBtn.Size = new System.Drawing.Size(205, 90);
            this.ErrorBtn.TabIndex = 3;
            this.ErrorBtn.Text = "Get Error Message";
            this.ErrorBtn.UseVisualStyleBackColor = true;
            this.ErrorBtn.Click += new System.EventHandler(this.ErrorBtn_Click);
            // 
            // InfoBtn
            // 
            this.InfoBtn.Location = new System.Drawing.Point(538, 198);
            this.InfoBtn.Name = "InfoBtn";
            this.InfoBtn.Size = new System.Drawing.Size(205, 90);
            this.InfoBtn.TabIndex = 4;
            this.InfoBtn.Text = "Info";
            this.InfoBtn.UseVisualStyleBackColor = true;
            this.InfoBtn.Click += new System.EventHandler(this.InfoBtn_Click);
            // 
            // ErasePartitionBtn
            // 
            this.ErasePartitionBtn.Location = new System.Drawing.Point(287, 198);
            this.ErasePartitionBtn.Name = "ErasePartitionBtn";
            this.ErasePartitionBtn.Size = new System.Drawing.Size(205, 90);
            this.ErasePartitionBtn.TabIndex = 5;
            this.ErasePartitionBtn.Text = "Erase_Partition";
            this.ErasePartitionBtn.UseVisualStyleBackColor = true;
            this.ErasePartitionBtn.Click += new System.EventHandler(this.ErasePartitionBtn_Click);
            // 
            // EraseByRegisterBtn
            // 
            this.EraseByRegisterBtn.Location = new System.Drawing.Point(27, 350);
            this.EraseByRegisterBtn.Name = "EraseByRegisterBtn";
            this.EraseByRegisterBtn.Size = new System.Drawing.Size(205, 90);
            this.EraseByRegisterBtn.TabIndex = 7;
            this.EraseByRegisterBtn.Text = "Erase By Register";
            this.EraseByRegisterBtn.UseVisualStyleBackColor = true;
            this.EraseByRegisterBtn.Click += new System.EventHandler(this.EraseByRegisterBtn_Click);
            // 
            // WriteUICRBtn
            // 
            this.WriteUICRBtn.Location = new System.Drawing.Point(287, 350);
            this.WriteUICRBtn.Name = "WriteUICRBtn";
            this.WriteUICRBtn.Size = new System.Drawing.Size(205, 90);
            this.WriteUICRBtn.TabIndex = 8;
            this.WriteUICRBtn.Text = "Write UICR";
            this.WriteUICRBtn.UseVisualStyleBackColor = true;
            this.WriteUICRBtn.Click += new System.EventHandler(this.WriteUICRBtn_Click);
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(538, 350);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(205, 90);
            this.button1.TabIndex = 9;
            this.button1.Text = "Read FICR Device ID ";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(9F, 18F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 530);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.WriteUICRBtn);
            this.Controls.Add(this.EraseByRegisterBtn);
            this.Controls.Add(this.ErasePartitionBtn);
            this.Controls.Add(this.InfoBtn);
            this.Controls.Add(this.ErrorBtn);
            this.Controls.Add(this.ResetBtn);
            this.Controls.Add(this.EraseBtn);
            this.Controls.Add(this.BurnBtn);
            this.Name = "Form1";
            this.Text = "Form1";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button BurnBtn;
        private System.Windows.Forms.Button EraseBtn;
        private System.Windows.Forms.Button ResetBtn;
        private System.Windows.Forms.Button ErrorBtn;
        private System.Windows.Forms.Button InfoBtn;
        private System.Windows.Forms.Button ErasePartitionBtn;
        private System.Windows.Forms.Button EraseByRegisterBtn;
        private System.Windows.Forms.Button WriteUICRBtn;
        private System.Windows.Forms.Button button1;
    }
}

