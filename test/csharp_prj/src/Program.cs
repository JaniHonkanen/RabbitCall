using System;
using System.Globalization;
using CsNamespace;

public unsafe class Program {

	static void Main(string[] args) {
		CultureInfo customCulture = new CultureInfo("en-US");
		customCulture.NumberFormat.NumberDecimalSeparator = ".";
		CultureInfo.DefaultThreadCurrentCulture = customCulture;
		CultureInfo.DefaultThreadCurrentUICulture = customCulture;

		bool openGlTestEnabled = true;
		foreach (string arg in args) {
			if (arg == "-skipOpenGlTest") openGlTestEnabled = false;
		}

		RabbitCallApi.init();

		new FunctionalTests().run(openGlTestEnabled);
		Log.write("");
		new PerformanceTests().run();
	}
}

