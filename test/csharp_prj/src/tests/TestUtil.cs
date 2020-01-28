
using System;
using System.Collections.Generic;

public class TestUtil {

	public static void checkEqual<T>(T o1, T o2, string description) {
		if (!Equals(o1, o2)) throw new Exception($"{description} ({o1} vs {o2})");
	}

}
