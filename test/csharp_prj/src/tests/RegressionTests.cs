using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

public class RegressionTests {

	public void run(string projectDir) {
		try {
			string baseHeaderFile = "rabbitcall.h";

			string[] generatedDirs = {
				Path.GetFullPath(Path.Combine(projectDir, "cpp_prj/src/rabbitcall")),
				Path.GetFullPath(Path.Combine(projectDir, "csharp_prj/src/rabbitcall"))
			};

			string expectedDir = Path.GetFullPath(Path.Combine(projectDir, "test_data/expected_generated_files"));

			SortedSet<string> processedFiles = new SortedSet<string>();
			foreach (string generatedDir in generatedDirs) {
				foreach (string generatedFile in Directory.EnumerateFiles(generatedDir)) {
					//Log.write($"Checking file for regression: {generatedFile}");
					string filename = Path.GetFileName(generatedFile);
					if (filename != baseHeaderFile) {
						string expectedFile = Path.Combine(expectedDir, filename);
						if (!File.Exists(expectedFile)) throw new Exception($"An extra file found in generated directory ({generatedFile}) that does not exist in expected files directory ({expectedDir})");

						byte[] generatedFileData = File.ReadAllBytes(generatedFile);
						byte[] expectedFileData = File.ReadAllBytes(expectedFile);

						if (!generatedFileData.SequenceEqual(expectedFileData)) {
							throw new Exception($"Possible regression: generated file ({generatedFile}) differs from old version of the same file ({expectedFile}). If the change is intentional, update the old file to make the test pass.");
						}

						if (!processedFiles.Add(filename)) {
							throw new Exception($"Same generated file found in multiple generated-file directories: {filename}");
						}
					}
				}
			}

			foreach (string expectedFile in Directory.EnumerateFiles(expectedDir)) {
				string filename = Path.GetFileName(expectedFile);
				if (!processedFiles.Contains(filename)) throw new Exception($"Expected file {filename} to be generated. If you modified the tool so that this file is no longer generated, remove it from the expected-files directory {expectedDir}");
			}

			if (processedFiles.Count < 1) throw new Exception($"Fewer generated files found than expected: {processedFiles.Count}");

			Log.write("*** REGRESSION TEST SUCCESS ***", null, ConsoleColor.Green);
		}
		catch (Exception e) {
			Log.write("*** REGRESSION TEST FAILED ***", e, ConsoleColor.Red);
		}
	}

}
