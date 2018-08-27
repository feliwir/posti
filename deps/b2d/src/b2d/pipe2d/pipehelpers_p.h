// [B2dPipe]
// 2D Pipeline Compiler.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// [Guard]
#ifndef _B2D_PIPE2D_PIPEHELPERS_P_H
#define _B2D_PIPE2D_PIPEHELPERS_P_H

// [Dependencies]
#include "./pipeglobals_p.h"

B2D_BEGIN_SUB_NAMESPACE(Pipe2D)

//! \addtogroup b2d_pipe2d
//! \{

// ============================================================================
// [b2d::Pipe2D::AsmJit-Integration]
// ============================================================================

namespace x86 {
  using namespace ::asmjit::x86;
}

using asmjit::Label;
using asmjit::Operand;
using asmjit::Operand_;

// ============================================================================
// [b2d::Pipe2D::RegV]
// ============================================================================

class OpArray {
public:
  enum { kMaxSize = 4 };

  inline OpArray() noexcept { reset(); }
  inline explicit OpArray(const Operand_& op0) noexcept { init(op0); }
  inline OpArray(const Operand_& op0, const Operand_& op1) noexcept { init(op0, op1); }
  inline OpArray(const Operand_& op0, const Operand_& op1, const Operand_& op2) noexcept { init(op0, op1, op2); }
  inline OpArray(const Operand_& op0, const Operand_& op1, const Operand_& op2, const Operand_& op3) noexcept { init(op0, op1, op2, op3); }
  inline OpArray(const OpArray& other) noexcept { init(other); }

protected:
  // Used internally to implement `low()`, `high()`, `even()`, and `odd()`.
  inline OpArray(const OpArray& other, uint32_t from, uint32_t inc, uint32_t limit) noexcept {
    uint32_t di = 0;
    for (uint32_t si = from; si < limit; si += inc)
      v[di++] = other[si];
    _size = di;
  }

public:
  inline void init(const Operand_& op0) noexcept {
    _size = 1;
    v[0] = op0;
  }

  inline void init(const Operand_& op0, const Operand_& op1) noexcept {
    _size = 2;
    v[0] = op0;
    v[1] = op1;
  }

  inline void init(const Operand_& op0, const Operand_& op1, const Operand_& op2) noexcept {
    _size = 3;
    v[0] = op0;
    v[1] = op1;
    v[2] = op2;
  }

  inline void init(const Operand_& op0, const Operand_& op1, const Operand_& op2, const Operand_& op3) noexcept {
    _size = 4;
    v[0] = op0;
    v[1] = op1;
    v[2] = op2;
    v[3] = op3;
  }

  inline void init(const OpArray& other) noexcept {
    _size = other._size;
    if (_size)
      std::memcpy(v, other.v, _size * sizeof(Operand_));
  }

  //! Reset to the construction state.
  inline void reset() noexcept { _size = 0; }

  //! Get whether the vector is empty (has no elements).
  inline bool empty() const noexcept { return _size == 0; }
  //! Get whether the vector has only one element, which makes it scalar.
  inline bool isScalar() const noexcept { return _size == 1; }
  //! Get whether the vector has more than 1 element, which means that
  //! calling `high()` and `odd()` won't return an empty vector.
  inline bool isVector() const noexcept { return _size > 1; }

  //! Get number of vector elements.
  inline uint32_t size() const noexcept { return _size; }

  inline Operand_& operator[](size_t index) noexcept {
    B2D_ASSERT(index < _size);
    return v[index];
  }

  inline const Operand_& operator[](size_t index) const noexcept {
    B2D_ASSERT(index < _size);
    return v[index];
  }

  inline OpArray lo() const noexcept { return OpArray(*this, 0, 1, (_size + 1) / 2); }
  inline OpArray hi() const noexcept { return OpArray(*this, _size > 1 ? (_size + 1) / 2 : 0, 1, _size); }
  inline OpArray even() const noexcept { return OpArray(*this, 0, 2, _size); }
  inline OpArray odd() const noexcept { return OpArray(*this, _size > 1, 2, _size); }

  //! Return a new vector consisting of either even (from == 0) or odd (from == 1)
  //! elements. It's like calling `even()` and `odd()`, but can be used within a
  //! loop that performs the same operation for both.
  inline OpArray even_odd(uint32_t from) const noexcept { return OpArray(*this, _size > 1 ? from : 0, 2, _size); }

  uint32_t _size;
  Operand_ v[kMaxSize];
};

template<typename T>
class OpArrayT : public OpArray {
public:
  inline OpArrayT() noexcept : OpArray() {}
  inline explicit OpArrayT(const T& op0) noexcept : OpArray(op0) {}
  inline OpArrayT(const T& op0, const T& op1) noexcept : OpArray(op0, op1) {}
  inline OpArrayT(const T& op0, const T& op1, const T& op2) noexcept : OpArray(op0, op1, op2) {}
  inline OpArrayT(const T& op0, const T& op1, const T& op2, const T& op3) noexcept : OpArray(op0, op1, op2, op3) {}
  inline OpArrayT(const OpArrayT<T>& other) noexcept : OpArray(other) {}

protected:
  inline OpArrayT(const OpArrayT<T>& other, uint32_t n, uint32_t from, uint32_t inc) noexcept : OpArray(other, n, from, inc) {}

public:
  inline void init(const T& op0) noexcept { OpArray::init(op0); }
  inline void init(const T& op0, const T& op1) noexcept { OpArray::init(op0, op1); }
  inline void init(const T& op0, const T& op1, const T& op2) noexcept { OpArray::init(op0, op1, op2); }
  inline void init(const T& op0, const T& op1, const T& op2, const T& op3) noexcept { OpArray::init(op0, op1, op2, op3); }
  inline void init(const OpArrayT<T>& other) noexcept { OpArray::init(other); }

  inline T& operator[](size_t index) noexcept {
    B2D_ASSERT(index < _size);
    return static_cast<T&>(v[index]);
  }

  inline const T& operator[](size_t index) const noexcept {
    B2D_ASSERT(index < _size);
    return static_cast<const T&>(v[index]);
  }

  inline OpArrayT<T> lo() const noexcept { return OpArrayT<T>(*this, 0, 1, (_size + 1) / 2); }
  inline OpArrayT<T> hi() const noexcept { return OpArrayT<T>(*this, _size > 1 ? (_size + 1) / 2 : 0, 1, _size); }
  inline OpArrayT<T> even() const noexcept { return OpArrayT<T>(*this, 0, 2, _size); }
  inline OpArrayT<T> odd() const noexcept { return OpArrayT<T>(*this, _size > 1, 2, _size); }
  inline OpArrayT<T> even_odd(uint32_t from) const noexcept { return OpArrayT<T>(*this, _size > 1 ? from : 0, 2, _size); }
};

typedef OpArrayT<x86::Vec> VecArray;

// ============================================================================
// [b2d::Pipe2D::OpAccess]
// ============================================================================

template<typename T>
struct OpAccessImpl {
  typedef T Input;
  typedef T Output;

  static inline uint32_t size(const Input& op) noexcept { return 1; }
  static inline Output& at(Input& op, size_t i) noexcept { return op; }
  static inline const Output& at(const Input& op, size_t i) noexcept { return op; }
};

template<>
struct OpAccessImpl<OpArray> {
  typedef OpArray Input;
  typedef Operand_ Output;

  static inline uint32_t size(const Input& op) noexcept { return op.size(); }
  static inline Output& at(Input& op, size_t i) noexcept { return op[i]; }
  static inline const Output& at(const Input& op, size_t i) noexcept { return op[i]; }
};

struct OpAccess {
  template<typename OpT>
  static inline uint32_t opCount(const OpT& op) noexcept { return OpAccessImpl<OpT>::size(op); }
  template<typename OpT>
  static inline typename OpAccessImpl<OpT>::Output& at(OpT& op, size_t i) noexcept { return OpAccessImpl<OpT>::at(op, i); }
  template<typename OpT>
  static inline const typename OpAccessImpl<OpT>::Output& at(const OpT& op, size_t i) noexcept { return OpAccessImpl<OpT>::at(op, i); }
};

// ============================================================================
// [b2d::Pipe2D::PipeScopedInject]
// ============================================================================

class PipeScopedInject {
public:
  B2D_NONCOPYABLE(PipeScopedInject)

  inline PipeScopedInject(asmjit::BaseCompiler* cc, asmjit::BaseNode** hook) noexcept
    : cc(cc),
      hook(hook),
      prev(cc->setCursor(*hook)) {}

  inline ~PipeScopedInject() noexcept {
    *hook = cc->setCursor(prev);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  asmjit::BaseCompiler* cc;
  asmjit::BaseNode** hook;
  asmjit::BaseNode* prev;
};

// ============================================================================
// [b2d::Pipe2D::JitUtils]
// ============================================================================

//! Utilities used by PipeCompiler and other parts of the library.
struct JitUtils {
  template<typename T>
  static inline void resetVarArray(T* array, size_t size) noexcept {
    for (size_t i = 0; i < size; i++)
      array[i].reset();
  }

  template<typename T>
  static inline void resetVarStruct(T* data, size_t size = sizeof(T)) noexcept {
    resetVarArray(reinterpret_cast<asmjit::BaseReg*>(data), size / sizeof(asmjit::BaseReg));
  }
};

// ============================================================================
// [b2d::Pipe2D::PipeCMask]
// ============================================================================

//! A constant mask (CMASK) stored in either GP or XMM register.
struct PipeCMask {
  // --------------------------------------------------------------------------
  // [Reset]
  // --------------------------------------------------------------------------

  inline void reset() noexcept {
    JitUtils::resetVarStruct<PipeCMask>(this);
  }

  // --------------------------------------------------------------------------
  // [Members]
  // --------------------------------------------------------------------------

  struct Gp {
    //! Mask scalar [0...256].
    x86::Gp m;
    //! Inverted mask `256 - m` scalar [0...256].
    x86::Gp im;
  } gp;

  struct Vec {
    //! Mask expanded to a vector of uint16_t quantities [0...256].
    x86::Vec m;
    //! Inverted mask `256 - m` expanded to a vector of uint16_t quantities [0...256].
    x86::Vec im;
  } vec;
};

//! \}

B2D_END_SUB_NAMESPACE

// [Guard]
#endif // _B2D_PIPE2D_PIPEHELPERS_P_H
