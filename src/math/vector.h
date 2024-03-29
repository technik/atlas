//-------------------------------------------------------------------------------------------------
// Toy path tracer
//-------------------------------------------------------------------------------------------------
// Copyright 2018 Carmelo J Fdez-Aguera
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software
// and associated documentation files (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge, publish, distribute,
// sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
// NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#pragma once

#include <initializer_list>
#include <cmath>

namespace math
{
	template<class T, int n>
	struct Vector
	{
	public:
		Vector() = default;
		Vector(std::initializer_list<T> il)
		{
			auto iter = il.begin();
			for(size_t i = 0; i < n; ++i)
				m[i] = *iter++;
		}
		constexpr Vector(T t) {
			for(auto i = 0; i < n; ++i)
				m[i] = t;
		}
		constexpr Vector(T x, T y) : m {x,y} {}
		constexpr Vector(T x, T y, T z) : m {x,y,z} {}
		constexpr Vector(T x, T y, T z, T w) : m {x,y,z,w} {}

		/*Vector(const std::array<T, n>& v) {
			for(auto i = 0; i < n; ++i)
				m[i] = v[i];
			return *this;
		}*/

		Vector& operator=(const Vector& v) = default;
		/*Vector& operator=(const std::array<T, n>& v) {
			for(auto i = 0; i < n; ++i)
				m[i] = v[i];
			return *this;
		}*/

		// Vector accessors
		T x() const { return m[0]; }
		T y() const { return m[1]; static_assert(n>0); }
		T z() const { return m[2]; static_assert(n>1); }
		T w() const { return m[2]; static_assert(n>2); }
		T& x() { return m[0]; }
		T& y() { return m[1]; static_assert(n>0); }
		T& z() { return m[2]; static_assert(n>1); }
		T& w() { return m[2]; static_assert(n>2); }

		// Indexed accessor
		constexpr T operator[](size_t i) const { return m[i]; }
		T& operator[](size_t i) { return m[i]; }

		// Basic properties
		constexpr T norm() const { return std::sqrt(sqNorm()); }
		constexpr T sqNorm() const;

		// Math operators
		Vector operator-() const { return {-x(), -y(), -z()}; }
		Vector& operator+=(const Vector& v) {
			for(size_t i = 0; i < n; ++i)
				m[i] += v.m[i];
			return *this;
		}

		Vector& operator-=(const Vector& v) {
			for(size_t i = 0; i < n; ++i)
				m[i] -= v.m[i];
			return *this;
		}

		Vector& operator*=(const Vector& v) {
			for(size_t i = 0; i < n; ++i)
				m[i] *= v.m[i];
			return *this;
		}

		Vector& operator/=(const Vector& v) {
			for(size_t i = 0; i < n; ++i)
				m[i] /= v.m[i];
			return *this;
		}

		template<class T2>
		Vector& operator*=(T2 t) {
			for(size_t i = 0; i < n; ++i)
				m[i] *= T(t);
			return *this;
		}

		template<class T2>
		Vector& operator/=(T2 t) {
			for(size_t i = 0; i < n; ++i)
				m[i] /= T(t);
			return *this;
		}

	private:
		T m[n];
	};

	//---------------------------------------------------------------------------------------------
	// Useful aliases
	//---------------------------------------------------------------------------------------------

	template<class T> using Vector2 = Vector<T,2>;
	template<class T> using Vector3 = Vector<T,3>;
	template<class T> using Vector4 = Vector<T,4>;

	using Vec2f = Vector2<float>;
	using Vec3f = Vector3<float>;
	using Vec4f = Vector4<float>;

	using Vec2d = Vector2<double>;
	using Vec3d = Vector3<double>;
	using Vec4d = Vector4<double>;

	//---------------------------------------------------------------------------------------------
	// External operators
	//---------------------------------------------------------------------------------------------
	template<class T, int n>
	Vector<T,n> operator+(const Vector<T,n>& a, const Vector<T,n>& b)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = a[i] + b[i];
		return res;
	}

	template<class T, int n>
	Vector<T,n> operator-(const Vector<T,n>& a, const Vector<T,n>& b)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = a[i] - b[i];
		return res;
	}

	template<class T, int n>
	Vector<T,n> operator*(const Vector<T,n>& a, const Vector<T,n>& b)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = a[i] * b[i];
		return res;
	}

	template<class T, int n>
	Vector<T,n> operator/(const Vector<T,n>& a, const Vector<T,n>& b)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = a[i] / b[i];
		return res;
	}

	template<class T, int n>
	Vector<T,n> operator+(const Vector<T,n>& a, float b)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = a[i] + b;
		return res;
	}

	template<class T, int n>
	Vector<T,n> operator-(const Vector<T,n>& a, float b)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = a[i] - b;
		return res;
	}

	template<class T, int n>
	Vector<T,n> operator+(float b, const Vector<T,n>& a)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = a[i] + b;
		return res;
	}

	template<class T, int n>
	Vector<T,n> operator-(float b, const Vector<T,n>& a)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = a[i] - b;
		return res;
	}

	template<class T, int n>
	Vector<T,n> operator*(const Vector<T,n>& a, float b)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = a[i] * b;
		return res;
	}

	template<class T, int n>
	Vector<T,n> operator/(const Vector<T,n>& a, float b)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = a[i] / b;
		return res;
	}

	template<class T, int n>
	Vector<T,n> operator*(float b, const Vector<T,n>& a)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = a[i] * b;
		return res;
	}

	template<class T, int n>
	Vector<T,n> operator/(float b, const Vector<T,n>& a)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = a[i] / b;
		return res;
	}

	template<class T, int n>
	bool operator==(const Vector<T,n>& a, const Vector<T,n>& b)
	{
		for(int i = 0; i < n; ++i)
			if(!(a[i] == b[i]))
				return false;
		return true;
	}

	//---------------------------------------------------------------------------------------------
	// Inline methods
	//---------------------------------------------------------------------------------------------
	template<class T, int n> auto dot(const Vector<T,n>& a, const Vector<T,n>& b)
	{
		auto d = T(0);
		for(int i = 0; i < n; ++i)
			d += a[i]*b[i];
		return d;
	}

	template<class T>
	Vector<T,3> cross(const Vector<T,3>& a, const Vector<T,3>& b)
	{
		return {
			a.y()*b.z()-a.z()*b.y(),
			a.z()*b.x()-a.x()*b.z(),
			a.x()*b.y()-a.y()*b.x()
		};
	}

	template<class T, int n>
	auto reflect(const Vector<T,n>& v, const Vector<T,n>& normal)
	{
		return v - 2 * dot(v, normal) * normal;
	}

	template<class T, int n>
	auto min(const Vector<T,n>& a, const Vector<T,n>& b)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = std::min(a[i], b[i]);
		return res;
	}

	template<class T, int n>
	auto max(const Vector<T,n>& a, const Vector<T,n>& b)
	{
		Vector<T,n> res;
		for(int i = 0; i < n; ++i)
			res[i] = std::max(a[i], b[i]);
		return res;
	}

    template<class T, int n>
    auto abs(const Vector<T, n>& a)
    {
        Vector<T, n> res;
        for (int i = 0; i < n; ++i)
            res[i] = std::abs(a[i]);
        return res;
    }

	template<class T, int n>
	constexpr T Vector<T,n>::sqNorm() const {
		return dot(*this, *this);
	}

	template<class T, int n> auto normalize(const Vector<T,n>& v)
	{
		return v * (1/v.norm());
	}

	// Vec3f specializations
	__forceinline Vector<float, 3> operator+(const Vector<float, 3>& a, const Vector<float, 3>& b)
	{
		return Vector<float, 3>(a.x() + b.x(), a.y() + b.y(), a.z() + b.z());
	}

	__forceinline Vector<float, 3> operator-(const Vector<float, 3>& a, const Vector<float, 3>& b)
	{
		return Vector<float, 3>(a.x() - b.x(), a.y() - b.y(), a.z() - b.z());
	}

	__forceinline Vector<float, 3> operator*(const Vector<float, 3>& a, const Vector<float, 3>& b)
	{
		return Vector<float, 3>(a.x() * b.x(), a.y() * b.y(), a.z() * b.z());
	}

	__forceinline Vector<float, 3> operator*(const Vector<float, 3>& a, float b)
	{
		return Vector<float, 3>(a.x() * b, a.y() * b, a.z() * b);
	}

	__forceinline Vector<float, 3> operator/(const Vector<float, 3>& a, float b)
	{
		auto rcp = 1.f / b;
		return Vector<float, 3>(a.x() * rcp, a.y() * rcp, a.z() * rcp);
	}

}	// namespace math