
using System;

public class Log {

	public static Action<string> outputWriter = Console.WriteLine;

	public static void write(string s, Exception e = null, ConsoleColor? color = null) {
		if (color != null) {
			Console.ForegroundColor = color.Value;
		}

		if (s != null) {
			outputWriter(s);
		}

		if (e != null) {
			bool first = true;
			for (Exception exc = e; exc != null; exc = exc.InnerException) {
				outputWriter((first ? "" : "Caused by: ") + exc.Message);
				outputWriter(exc.StackTrace);
				first = false;
			}
		}

		if (color != null) {
			Console.ResetColor();
		}
	}

}
