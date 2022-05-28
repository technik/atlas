//-------------------------------------------------------------------------------------------------
// Toy path tracer
//--------------------------------------------------------------------------------------------------
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

#include <cassert>
#include <initializer_list>
#include "aabb.h"

#include <DirectXMath.h>

namespace math
{
	class alignas(4 * sizeof(float)) Matrix34f
	{
	public:
		Matrix34f() = default;
		Matrix34f(std::initializer_list<float> il)
		{
			assert(il.size() == 12);

			auto iter = il.begin();
			for (size_t i = 0; i < il.size(); ++i)
				m[i] = *iter++;
		}

		Matrix34f(float x)
		{
			for(auto i = 0; i < 12; ++i)
				m[i] = x;
		}

		Matrix34f inverse() const;

		static Matrix34f identity()
		{
			Matrix34f x;
			for(int i = 0; i < 3; ++i)
				for(int j = 0; j < 4; ++j)
					x(i,j) = float(i==j?1:0);
			return x;
		}

		Vec3f& position() { return reinterpret_cast<Vec3f&>((*this)(0,3)); }
		Vec3f position() const {
			return col<3>();
		}

		Matrix34f operator*(const Matrix34f& b) const
		{
			Matrix34f res;
			for(int i = 0; i < 3; ++i)
			{
				for(int j = 0; j < 4; ++j)
				{
					res(i,j) =
						(*this)(i,0)*b(0,j) +
						(*this)(i,1)*b(1,j) +
						(*this)(i,2)*b(2,j);
				}
				res(i,3) += (*this)(i,3);
			}
			return res;
		}

        AABB operator*(const AABB& b) const
        {
            Vec3f origin = b.origin();
            Vec3f halfSize = b.max() - origin; // Positive by definition

            Vec3f ex = abs(col<0>() * halfSize.x());
            Vec3f ey = abs(col<1>() * halfSize.y());
            Vec3f ez = abs(col<2>() * halfSize.z());

            Vec3f extent = ex + ey + ez;

            origin = transformPos(origin);
            auto newMax = origin + extent;
            auto newMin = origin - extent;

            return AABB(newMin, newMax);
        }

		Vec3f transformPos(const Vec3f& v) const
		{
			Vec3f res;
			for(int i = 0; i < 3; ++i)
			{
				res[i] =
					(*this)(i,0)*v[0] +
					(*this)(i,1)*v[1] +
					(*this)(i,2)*v[2] +
					(*this)(i,3);
			}
			return res;
		}

        template<int i>
        const Vec3f& col() const
        {
            static_assert(i < 4);
            return reinterpret_cast<const Vec3f&>(m[3 * i]);
        }

        template<int i>
        Vec3f& col()
        {
            static_assert(i < 4);
            return reinterpret_cast<Vec3f&>(m[3 * i]);
        }

		Vec3f transformDir(const Vec3f& v) const
		{
			Vec3f res;
			for(int i = 0; i < 3; ++i)
			{
				res[i] =
					(*this)(i,0)*v[0] +
					(*this)(i,1)*v[1] +
					(*this)(i,2)*v[2];
			}
			return res;
		}

		float& operator()(int i, int j)
		{
			return m[3*j+i];
		}

		float operator()(int i, int j) const
		{
			return m[3*j+i];
		}

	private:
		// Column major storage
		float m[12];
	};

	class alignas(16 * sizeof(float)) Matrix44f
	{
	public:
		Matrix44f() = default;

		// Construct from a row major initializer list
		Matrix44f(std::initializer_list<float> il)
		{
			assert(il.size() == 16);

			auto iter = il.begin();
			float* data = &m_rows[0][0];
			for (size_t i = 0; i < il.size(); ++i)
				data[i] = *iter++;
		}
		Matrix44f(const Matrix44f&) = default;

		// Assuming an implicit (0,0,0,1) 4th row
		explicit Matrix44f(const Matrix34f& x)
		{
			*this = Matrix44f::identity();
			for (int i = 0; i < 3; ++i)
				for (int j = 0; j < 4; ++j)
					(*this)(i, j) = x(i, j);
		}

		static Matrix44f FromColMajorArray(const std::array<float,16>& colMajorArray)
		{
			Matrix44f result;
			for (int i = 0; i < 4; ++i)
			{
				for (int j = 0; j < 4; ++j)
					result(i, j) = colMajorArray[4 * j + i];
			}
			return result;
		}

		static Matrix44f FromRowMajorArray(const std::array<float, 16>& rowMajorArray)
		{
			Matrix44f result;
			memcpy(result.m_rows, rowMajorArray.data(), sizeof(Matrix44f));
			return result;
		}

		explicit Matrix44f(const DirectX::XMMATRIX& dxMatrix)
		{
			memcpy(this, &dxMatrix, sizeof(DirectX::XMMATRIX));
		}

		explicit operator DirectX::XMMATRIX() const
		{
			return reinterpret_cast<const DirectX::XMMATRIX&>(*this);
		}

		bool operator== (const Matrix44f& x) const
		{
			for(int i = 0; i < 3; ++i)
				for(int j = 0; j < 4; ++j)
					if((*this)(i,j) != x(i,j)) return false;
			return true;
		}

		static Matrix44f identity()
		{
			Matrix44f x;
			for(int i = 0; i < 4; ++i)
				for(int j = 0; j < 4; ++j)
					x(i,j) = float(i==j?1:0);
			return x;
		}

		Matrix44f operator*(const Matrix44f& b) const
		{
			Matrix44f res;
			for(int i = 0; i < 4; ++i)
			{
				for(int j = 0; j < 4; ++j)
				{
					res(i,j) =
						(*this)(i,0)*b(0,j) +
						(*this)(i,1)*b(1,j) +
						(*this)(i,2)*b(2,j) +
						(*this)(i,3)*b(3,j);
				}
			}
			return res;
		}

		Vec4f operator*(const Vec4f& b) const
		{
			Vec4f res;
			for(int i = 0; i < 4; ++i)
			{
				res[i] =
					(*this)(i,0)*b[0] +
					(*this)(i,1)*b[1] +
					(*this)(i,2)*b[2] +
					(*this)(i,3)*b[3];
			}
			return res;
		}

		template<size_t i>
		auto& row() { return m_rows[i]; }
		template<size_t i> 
		constexpr auto row() const { return m_rows[i]; }
		Vec4f& row(size_t i) { return m_rows[i]; }
		constexpr Vec4f row(size_t i) const { return m_rows[i]; }

		float& operator()(int i, int j)
		{
			return m_rows[i][j];
		}

		constexpr float operator()(int i, int j) const
		{
			return m_rows[i][j];
		}

		constexpr float element(int i, int j) const
		{
			return m_rows[i][j];
		}

	private:
		// Column major storage
		Vec4f m_rows[4];
	};

	inline Matrix44f transpose(const Matrix44f x)
	{
		Matrix44f result;
		for (int i = 0; i < 4; ++i)
		{
			for (int j = 0; j < 4; ++j)
			{
				result(i,j) = x(j,i);
			}
		}
		return result;
	}

	inline auto operator*(const Vec4f& v, const Matrix44f& m)
	{
		Vec4f res =
			v.x() * m.row<0>() +
			v.y() * m.row<1>() +
			v.z() * m.row<2>() +
			v.w() * m.row<3>();
		return res;
	}

	using Mat34f = Matrix34f;
	using Mat44f = Matrix44f;

} // namespace math
