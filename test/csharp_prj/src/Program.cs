using System;
using System.Globalization;
using CsNamespace;

public unsafe class Program {

	static void Main(string[] args) {
		CultureInfo customCulture = new CultureInfo("en-US");
		customCulture.NumberFormat.NumberDecimalSeparator = ".";
		CultureInfo.DefaultThreadCurrentCulture = customCulture;
		CultureInfo.DefaultThreadCurrentUICulture = customCulture;

		string projectDir = null;
		string projectDirParamName = "dir";
		bool openGlTestEnabled = true;
		for (int i = 0; i < args.Length; i++) {
			if (args[i] == "-skipOpenGlTest") {
				openGlTestEnabled = false;
			}
			else if (args[i] == $"-{projectDirParamName}") {
				if (i + 1 >= args.Length) throw new Exception($"Expected directory parameter after -{projectDirParamName}");
				projectDir = args[i + 1];
			}
		}

		if (projectDir == null) throw new Exception($"Command-line parameter -{projectDirParamName} expected");

		RabbitCallApi.init();

		Log.write("");
		new RegressionTests().run(projectDir);
		Log.write("");
		new FunctionalTests().run(openGlTestEnabled);
		Log.write("");
		new PerformanceTests().run();
	}
}

