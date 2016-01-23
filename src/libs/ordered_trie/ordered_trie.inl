/**
 * @file  ordered_trie.inl
 */

namespace ordered_trie {

template<typename MetaData,
         typename Suggestion = std::string,
         typename Encoding = encoders::v0::Impl>
class Parameters
{
  using encoding_type   = Encoding;
  using suggestion_type = Suggestion;
  using metadata_type   = MetaData;
  using NodeView = typename Encoding::View;
};

} // namespace ordered_trie {
