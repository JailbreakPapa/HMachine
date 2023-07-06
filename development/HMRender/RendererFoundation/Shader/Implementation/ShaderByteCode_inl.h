

const void* wdGALShaderByteCode::GetByteCode() const
{
  if (m_Source.IsEmpty())
    return nullptr;

  return &m_Source[0];
}

wdUInt32 wdGALShaderByteCode::GetSize() const
{
  return m_Source.GetCount();
}

bool wdGALShaderByteCode::IsValid() const
{
  return !m_Source.IsEmpty();
}
