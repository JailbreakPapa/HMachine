#pragma once

template <typename T>
WD_ALWAYS_INLINE wdAmbientCube<T>::wdAmbientCube()
{
  wdMemoryUtils::ZeroFillArray(m_Values);
}

template <typename T>
template <typename U>
WD_ALWAYS_INLINE wdAmbientCube<T>::wdAmbientCube(const wdAmbientCube<U>& other)
{
  *this = other;
}

template <typename T>
template <typename U>
WD_FORCE_INLINE void wdAmbientCube<T>::operator=(const wdAmbientCube<U>& other)
{
  for (wdUInt32 i = 0; i < wdAmbientCubeBasis::NumDirs; ++i)
  {
    m_Values[i] = other.m_Values[i];
  }
}

template <typename T>
WD_FORCE_INLINE bool wdAmbientCube<T>::operator==(const wdAmbientCube& other) const
{
  return wdMemoryUtils::IsEqual(m_Values, other.m_Values);
}

template <typename T>
WD_ALWAYS_INLINE bool wdAmbientCube<T>::operator!=(const wdAmbientCube& other) const
{
  return !(*this == other);
}

template <typename T>
void wdAmbientCube<T>::AddSample(const wdVec3& vDir, const T& value)
{
  m_Values[vDir.x > 0.0f ? 0 : 1] += wdMath::Abs(vDir.x) * value;
  m_Values[vDir.y > 0.0f ? 2 : 3] += wdMath::Abs(vDir.y) * value;
  m_Values[vDir.z > 0.0f ? 4 : 5] += wdMath::Abs(vDir.z) * value;
}

template <typename T>
T wdAmbientCube<T>::Evaluate(const wdVec3& vNormal) const
{
  wdVec3 vNormalSquared = vNormal.CompMul(vNormal);
  return vNormalSquared.x * m_Values[vNormal.x > 0.0f ? 0 : 1] + vNormalSquared.y * m_Values[vNormal.y > 0.0f ? 2 : 3] +
         vNormalSquared.z * m_Values[vNormal.z > 0.0f ? 4 : 5];
}

template <typename T>
wdResult wdAmbientCube<T>::Serialize(wdStreamWriter& inout_stream) const
{
  return inout_stream.WriteArray(m_Values);
}

template <typename T>
wdResult wdAmbientCube<T>::Deserialize(wdStreamReader& inout_stream)
{
  return inout_stream.ReadArray(m_Values);
}
