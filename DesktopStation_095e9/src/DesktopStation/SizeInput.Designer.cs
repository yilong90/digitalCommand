namespace DesktopStation
{
    partial class SizeInput
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
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
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.button_Cancel = new System.Windows.Forms.Button();
            this.button_Ok = new System.Windows.Forms.Button();
            this.numUpDown_Height = new System.Windows.Forms.NumericUpDown();
            this.numUpDown_Width = new System.Windows.Forms.NumericUpDown();
            this.label_S88DetectInterval = new System.Windows.Forms.Label();
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.numUpDown_Zoom = new System.Windows.Forms.NumericUpDown();
            ((System.ComponentModel.ISupportInitialize)(this.numUpDown_Height)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numUpDown_Width)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.numUpDown_Zoom)).BeginInit();
            this.SuspendLayout();
            // 
            // button_Cancel
            // 
            this.button_Cancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.button_Cancel.Location = new System.Drawing.Point(173, 110);
            this.button_Cancel.Name = "button_Cancel";
            this.button_Cancel.Size = new System.Drawing.Size(75, 33);
            this.button_Cancel.TabIndex = 9;
            this.button_Cancel.Text = "&Cancel";
            this.button_Cancel.UseVisualStyleBackColor = true;
            // 
            // button_Ok
            // 
            this.button_Ok.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.button_Ok.Location = new System.Drawing.Point(67, 110);
            this.button_Ok.Name = "button_Ok";
            this.button_Ok.Size = new System.Drawing.Size(75, 33);
            this.button_Ok.TabIndex = 8;
            this.button_Ok.Text = "&OK";
            this.button_Ok.UseVisualStyleBackColor = true;
            // 
            // numUpDown_Height
            // 
            this.numUpDown_Height.Font = new System.Drawing.Font("Arial", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(128)));
            this.numUpDown_Height.Location = new System.Drawing.Point(194, 21);
            this.numUpDown_Height.Maximum = new decimal(new int[] {
            128,
            0,
            0,
            0});
            this.numUpDown_Height.Minimum = new decimal(new int[] {
            14,
            0,
            0,
            0});
            this.numUpDown_Height.Name = "numUpDown_Height";
            this.numUpDown_Height.Size = new System.Drawing.Size(54, 23);
            this.numUpDown_Height.TabIndex = 11;
            this.numUpDown_Height.Value = new decimal(new int[] {
            14,
            0,
            0,
            0});
            // 
            // numUpDown_Width
            // 
            this.numUpDown_Width.Font = new System.Drawing.Font("Arial", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(128)));
            this.numUpDown_Width.Location = new System.Drawing.Point(67, 21);
            this.numUpDown_Width.Maximum = new decimal(new int[] {
            256,
            0,
            0,
            0});
            this.numUpDown_Width.Minimum = new decimal(new int[] {
            26,
            0,
            0,
            0});
            this.numUpDown_Width.Name = "numUpDown_Width";
            this.numUpDown_Width.Size = new System.Drawing.Size(54, 23);
            this.numUpDown_Width.TabIndex = 10;
            this.numUpDown_Width.Value = new decimal(new int[] {
            26,
            0,
            0,
            0});
            // 
            // label_S88DetectInterval
            // 
            this.label_S88DetectInterval.Location = new System.Drawing.Point(19, 24);
            this.label_S88DetectInterval.Name = "label_S88DetectInterval";
            this.label_S88DetectInterval.Size = new System.Drawing.Size(42, 13);
            this.label_S88DetectInterval.TabIndex = 12;
            this.label_S88DetectInterval.Text = "Width";
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(137, 24);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(42, 13);
            this.label1.TabIndex = 13;
            this.label1.Text = "Height";
            // 
            // label2
            // 
            this.label2.Location = new System.Drawing.Point(19, 71);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(42, 13);
            this.label2.TabIndex = 14;
            this.label2.Text = "Zoom";
            // 
            // numUpDown_Zoom
            // 
            this.numUpDown_Zoom.Font = new System.Drawing.Font("Arial", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(128)));
            this.numUpDown_Zoom.Increment = new decimal(new int[] {
            25,
            0,
            0,
            0});
            this.numUpDown_Zoom.Location = new System.Drawing.Point(67, 68);
            this.numUpDown_Zoom.Minimum = new decimal(new int[] {
            25,
            0,
            0,
            0});
            this.numUpDown_Zoom.Name = "numUpDown_Zoom";
            this.numUpDown_Zoom.Size = new System.Drawing.Size(112, 23);
            this.numUpDown_Zoom.TabIndex = 15;
            this.numUpDown_Zoom.Value = new decimal(new int[] {
            100,
            0,
            0,
            0});
            // 
            // SizeInput
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(278, 155);
            this.Controls.Add(this.numUpDown_Zoom);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.label_S88DetectInterval);
            this.Controls.Add(this.numUpDown_Height);
            this.Controls.Add(this.numUpDown_Width);
            this.Controls.Add(this.button_Cancel);
            this.Controls.Add(this.button_Ok);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "SizeInput";
            this.ShowIcon = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "New Layout Size";
            ((System.ComponentModel.ISupportInitialize)(this.numUpDown_Height)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numUpDown_Width)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.numUpDown_Zoom)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button button_Cancel;
        private System.Windows.Forms.Button button_Ok;
        public System.Windows.Forms.NumericUpDown numUpDown_Height;
        public System.Windows.Forms.NumericUpDown numUpDown_Width;
        private System.Windows.Forms.Label label_S88DetectInterval;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        public System.Windows.Forms.NumericUpDown numUpDown_Zoom;
    }
}