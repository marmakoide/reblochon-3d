#ifndef REBLOCHON_ARRAY_2D_H
#define REBLOCHON_ARRAY_2D_H

#include <ArrayT.h>



namespace reb {
	template <class T>
	class Array2dT {
	public:
		typedef T           value_type;
		typedef std::size_t size_type;



		inline Array2dT() :
			m_w(0),
			m_h(0),
			m_data(0) { }

		inline Array2dT(size_type w,
		                size_type h) :
			m_w(w),
			m_h(h),
			m_data(w * h) {
		}

		inline Array2dT(const Array2dT<T>& other) = default;

		inline size_type w() const {
			return m_w;
		}

		inline size_type h() const {
			return m_h;
		}

		inline ArrayT<T>& data() {
			return m_data;
		}

		inline const ArrayT<T>& data() const {
			return m_data;
		}

		

		inline value_type&
		operator () (int i, int j) {
			return m_data[index(i, j)];
		}

		inline const value_type&
		operator () (int i, int j) const {
			return m_data[index(i, j)];
		}

	private:
		size_type
		index(int i, int j) const {
			return m_w * j + i;
		}

		size_type m_w, m_h;
		ArrayT<T> m_data;
	}; // class Array2dT
} // namespace reb



#endif // REBLOCHON_ARRAY_2D_H
