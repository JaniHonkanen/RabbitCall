using System.Diagnostics.Contracts;

public struct Int4 {

	public int x, y, z, w;

	public Int4(int x, int y, int z, int w) {
		this.x = x;
		this.y = y;
		this.z = z;
		this.w = w;
	}

	[Pure]
	public bool Equals(Int4 other) {
		return x == other.x && y == other.y && z == other.z && w == other.w;
	}

	[Pure]
	public override bool Equals(object obj) {
		return obj is Int4 other && Equals(other);
	}

	[Pure]
	public override int GetHashCode() {
		unchecked {
			var hashCode = x;
			hashCode = (hashCode * 397) ^ y;
			hashCode = (hashCode * 397) ^ z;
			hashCode = (hashCode * 397) ^ w;
			return hashCode;
		}
	}

	[Pure]
	public override string ToString() {
		return $"[{x}; {y}; {z}; {w}]";
	}
}