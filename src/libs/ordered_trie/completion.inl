/**
 * @file  completion.inl
 */

namespace ordered_trie {

template<typename Parameters>
class Completion
{
public:

  using Suggestion = typename Parameters::suggestion_type;
  using MetaData   = typename Parameters::metadata_type;

  const Suggestion &suggestion () const
  {  
    return m_suggestion;
  };

  std::uint64_t score () const;
  {
    return m_score;
  };

  const MetaData &metadata () const
  {
    return m_metadata;
  };

  Completion (typename Parameters::NodeView leaf,
	      std::uint64_t     score,
	      boost::string_ref suggestion)
    : m_score (score)
    , m_suggestion (suggestion.begin (), suggestion.end ())
    , m_metadata (
       Serialise<MetaData>::deserialise (leaf.payload_data ()))
  {
  };
  
private:
  std::uint64_t m_score;     
  Suggestion    m_suggestion;  
  MetaData      m_metadata;
};

} // namespace ordered_trie

#endif
