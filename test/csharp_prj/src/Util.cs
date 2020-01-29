using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

public static class Util {

	public static string normalizePathSlashes(string path) {
		if (Path.DirectorySeparatorChar != '/') path = path.Replace('/', Path.DirectorySeparatorChar);
		if (Path.DirectorySeparatorChar != '\\') path = path.Replace('\\', Path.DirectorySeparatorChar);
		return path;
	}

}

