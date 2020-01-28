
using System.Diagnostics.Contracts;

public struct Double2 {

	public double x, y;

	public Double2(double x, double y) {
		this.x = x;
		this.y = y;
	}

	[Pure]
	public bool Equals(Double2 other) {
		return x.Equals(other.x) && y.Equals(other.y);
	}

	[Pure]
	public override bool Equals(object obj) {
		return obj is Double2 other && Equals(other);
	}

	[Pure]
	public override int GetHashCode() {
		unchecked {
			return (x.GetHashCode() * 397) ^ y.GetHashCode();
		}
	}

	[Pure]
	public override string ToString() {
		return $"[{x}; {y}]";
	}
}
