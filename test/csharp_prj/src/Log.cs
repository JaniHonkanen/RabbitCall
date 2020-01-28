
using System;

public class Log {

	public static void write(string s, Exception e = null, ConsoleColor? color = null) {
		if (color != null) {
			Console.ForegroundColor = color.Value;
		}

		if (s != null) {
			Console.WriteLine(s);
		}

		if (e != null) {
			bool first = true;
			for (Exception exc = e; exc != null; exc = exc.InnerException) {
				Console.WriteLine((first ? "" : "Caused by: ") + exc.Message);
				Console.WriteLine(exc.StackTrace);
				first = false;
			}
		}

		if (color != null) {
			Console.ResetColor();
		}
	}

}
