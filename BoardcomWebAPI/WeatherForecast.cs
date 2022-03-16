namespace ProjToshiba
{
    public class WeatherForecast
    {
        public DateTime Date { get; set; }

        public int TemperatureC { get; set; }

        public int TemperatureF => 32 + (int)(TemperatureC / 0.5556);

        public string? Summary { get; set; }

        /*input level*/
        public string bw = "";
        public string dataRate = "";
        public string channel = "";
        public string power = "";
        public string freq = "";

        /*output level*/
        public double peakPower { get; set; }
        public double avgPower { get; set; }
        public double evmAvg { get; set; }
        public double evmAvgPercentage { get; set; }
        public double freqCenterToleranceHz { get; set; }
        public double symbolClkFreqTolerancePPM { get; set; }

        public double APIgetResult(int powerlevel)
        {
            Feeddata_transmit(10, 11, 12);
            if (powerlevel >= 0)
            {

            }
            return 0;
        }


        const int Max = 100;

        public void Feeddata_transmit(double modul, double pa, double stability)
        {
            //int flag = 100; // count the loop times
            int[,,] formdata = new int[10, 10, 10];
            int[][][] intArr = new int[10][][];
            intArr[0] = new int[10][];
            intArr[1] = new int[10][];
            for (int i = -112, flag = 100; i < flag; i++)
            {
                for (int j = 6917; j < flag; j++)
                {
                    for (int k = -805; k < flag; k++)
                    {
                        i = i + j;
                        k = k + j;
                        //_ = formdata[i, j, k];
                        Console.WriteLine(Console.ReadLine());
                        Console.WriteLine(formdata[i, j, k]);
                    }

                }
                flag++;
            }
        }
        //static void Main(string[] args)
        //{

        //    GetCombination(new List<int> { 1, 2, 3 });
        //}

        static void GetCombination(List<int> list)
        {
            double count = Math.Pow(2, list.Count);
            for (int i = 1; i <= count - 1; i++)
            {
                string str = Convert.ToString(i, 2).PadLeft(list.Count, '0');
                for (int j = 0; j < str.Length; j++)
                {
                    if (str[j] == '1')
                    {
                        Console.Write(list[j]);
                    }
                }
                Console.WriteLine();
            }
        }


    }
}