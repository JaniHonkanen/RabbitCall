using System;
using System.Globalization;
using CsNamespace;

public unsafe class Program {

	static void Main(string[] args) {
		CultureInfo customCulture = new CultureInfo("en-US");
		customCulture.NumberFormat.NumberDecimalSeparator = ".";
		CultureInfo.DefaultThreadCurrentCulture = customCulture;
		CultureInfo.DefaultThreadCurrentUICulture = customCulture;

		RabbitCallApi.init();

		new FunctionalTests().run();
		Log.write("");
		new PerformanceTests().run();
	}
}

