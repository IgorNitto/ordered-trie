#ifndef DETAIL_ORDERED_TRIE_STORE_HPP
#define DETAIL_ORDERED_TRIE_STORE_HPP

/**
 * @file detail_ordered_trie_store.h
 */

#include "ordered_trie_node.hpp"
#include "ordered_trie_builtin_serialise.hpp"

#include <boost/range/algorithm.hpp>
#include <boost/utility/string_ref.hpp>

#include <iterator>
#include <string>
#include <fstream>
#include <memory>
#include <tuple>
#include <vector>

namespace ordered_trie {
namespace detail {

/**
 * Manage storage of ordered trie serialisation.
 */
template<typename Parameters>
class Store
{
public:
  static_assert (sizeof (char) == sizeof (std::uint8_t),
		 "Detected non 8-bit char platform");

  /**
   * Instantiate from file
   */
  static auto from_file (const std::string &path)
    -> std::shared_ptr<const Store>;

  /**
   * Instantiate from trie serialisation and optional
   * score indirection table
   */
  static auto from_memory (
    std::vector<std::uint8_t> serialised_trie,
    std::vector<std::uint8_t> serialised_score_table = {})
    -> std::shared_ptr<const Store>;
  
  /**
   * Write to file
   */
  void write (const std::string &path) const;

  /**
   * Get pointer to hosted trie serialisation (or nullptr if empty)
   */
  virtual auto trie_data () const
    -> std::pair<const std::uint8_t*,
                 const std::uint8_t*>;

  /**
   * Get pointer to hosted score indirection table serialisation
   * (or nullptr if absent).
   */
  virtual auto score_table_data () const
    -> std::pair<const std::uint8_t *,
                 const std::uint8_t *>;

  /**
   * Not-copyable or assignable
   */
  Store (const Store&) = delete;
  Store& operator=(const Store&) = delete;
  
  /**
   * Release number associated to current store
   */
  static auto release_number ()
    -> std::tuple <std::uint32_t,
                   std::uint32_t,
                   std::uint32_t>;

  ~Store () = default;

private:

  Store () = default;
  
  // @TODO: allow to memory-map the content of this store
  std::vector<std::uint8_t> m_serialised_trie;
  std::vector<std::uint8_t> m_serialised_score_table;
};

/*****************************************************************/
/* Inline implementation                                         */
/*****************************************************************/
 
enum class Endianness
{
  UNSPECIFIED,
  LITTLE,
  BIG
};

Endianness system_endianness ()
{
  if (*(reinterpret_cast<const std::uint16_t*> ("\x01\0")) == 1)
  {
    return Endianness::LITTLE;
  }
  else
  {
    return Endianness::BIG;
  }
}

/*****************************************************************/
/*
 * Make a string description of the score and metadata
 * types used for type-checking the file.
 */ 
template<typename Parameters>
const std::string& make_mangled_type_info ()
{
  const static auto mangled_type_info = []
  {
    return "ORDERED_TRIE_" +
            Parameters::ScoreSerialiser::format_id () +
            "\n";
  } ();

  return mangled_type_info;
}
  
/*****************************************************************/
/*
 * Store file header
 */
template<typename Parameters>
struct Header
{  
  /*
   * Endianness of encoding system
   */
  Endianness endianness = system_endianness ();

  /*
   * Potential payload data skipped in this version of header
   */
  std::size_t header_size;

  /*
   * Release number
   */
  std::uint32_t major_number =
     std::get<0> (Store<Parameters>::release_number ());
  
  std::uint32_t minor_number =
    std::get<1> (Store<Parameters>::release_number ());
  
  std::uint32_t patch_number =
    std::get<2> (Store<Parameters>::release_number ());

  /*
   * Score table offset in file (0 if absent)
   */
  std::pair<std::uint64_t, std::uint64_t>
  score_table_segment = std::make_pair (0, 0);
  
  /*
   * Score table offset in file (0 if absent)
   */
  std::pair<std::uint64_t, std::uint64_t>
  trie_segment = std::make_pair (0, 0);
};
  
/*****************************************************************/
/*
 * Header serialisation logic
 */
template<typename Parameters>
void serialise (std::vector<std::uint8_t> &out,
		const Header<Parameters> &header)
{
  using ordered_trie::serialise;
  
  /*
   * Write file initials
   */
  const auto &header_prefix = make_mangled_type_info<Parameters> ();
  boost::copy (header_prefix, std::back_inserter (out));

  /*
   * Serialise header structure to buffer
   */
  out.push_back (static_cast<std::uint8_t> (header.endianness));  
  serialise (out, header.major_number);
  serialise (out, header.minor_number);
  serialise (out, header.patch_number);
  serialise (out, header.score_table_segment.first);
  serialise (out, header.score_table_segment.second);
  serialise (out, header.trie_segment.first);
  serialise (out, header.trie_segment.second);
}

template<typename Parameters>
size_t serialised_header_size ()
{
  static const auto result = []
  {
    std::vector<std::uint8_t> test;
    serialise (test, Header<Parameters> {});
    return test.size ();
  } ();

  return result;
}
  
template<typename Parameters>
void put_header (std::ostream &os, const Header<Parameters> &header)
{
  std::vector<std::uint8_t> buffer;
  serialise (buffer, header);

  os.write (
    reinterpret_cast<const char*> (buffer.data ()),
    buffer.size ());

  if (!os)
  {
    throw std::logic_error ("Error while writing header");
  }
};

template<typename Parameters>
auto get_header (std::istream &binary_stream)
  -> Header<Parameters>
{
  /*
   * Read header (payload excluded)
   */
  std::vector<std::uint8_t> buffer (
    serialised_header_size<Parameters> (), 0);

  binary_stream.read (reinterpret_cast<char *> (buffer.data ()),
		      buffer.size ());

  if (!binary_stream)
  {
    throw std::logic_error ("Error reading file header");
  }
  
  /*
   * Validate mandatory initials and type info string
   */
  auto p = buffer.begin ();

  const auto &header_prefix =
    make_mangled_type_info<Parameters> ();

  if (!std::equal (header_prefix.begin (), header_prefix.end (), p))
  {
    throw std::invalid_argument ("Corrupt header or unexpected"
				 " stored types");
  }

  p += header_prefix.size ();

  if (static_cast<Endianness> (*p) != system_endianness ())
  {
    throw std::invalid_argument ("Incompatible system endianness"); 
  }

  Header<Parameters> result;

  result.endianness = system_endianness ();
  ++p;

  result.major_number = deserialise<std::uint32_t> (& (*p));
  p += sizeof (std::uint32_t);

  result.minor_number = deserialise<std::uint32_t> (& (*p));
  p += sizeof (std::uint32_t);
  
  result.patch_number = deserialise<std::uint32_t> (& (*p));
  p += sizeof (std::uint32_t);

  result.score_table_segment.first = deserialise<size_t> (& (*p));
  p += sizeof (size_t);

  result.score_table_segment.second = deserialise<size_t> (& (*p));
  p += sizeof (size_t);

  result.trie_segment.first = deserialise<size_t> (& (*p));
  p += sizeof (size_t);

  result.trie_segment.second = deserialise<size_t> (& (*p));
  p += sizeof (size_t);

  return result;
}

/*****************************************************************/

template<typename Parameters>
auto Store<Parameters>::release_number ()
 -> std::tuple <std::uint32_t, std::uint32_t, std::uint32_t>
{
  return std::make_tuple (1, 0, 0);
}

template<typename Parameters>
auto Store<Parameters>::from_memory (
  std::vector<std::uint8_t> serialised_trie,
  std::vector<std::uint8_t> serialised_score_table)
-> std::shared_ptr<const Store<Parameters>>
{
  std::shared_ptr<Store<Parameters>> result
    {new Store<Parameters> {}};

  result->m_serialised_trie = std::move (serialised_trie);
  result->m_serialised_score_table = std::move (serialised_score_table);

  return result;
}

template<typename Parameters>
auto Store<Parameters>::from_file (const std::string &path)
  -> std::shared_ptr<const Store<Parameters>>
{
  std::vector<std::uint8_t> serialised_score_table;
  std::vector<std::uint8_t> serialised_trie;  

  std::ifstream fin (path, std::ios_base::in |
		           std::ios_base::binary);

  fin.exceptions (std::ios_base::eofbit |
		  std::ios_base::badbit |
		  std::ios_base::failbit);
  
  /*
   * Load and validate header
   */ 
  const auto header = get_header<Parameters> (fin);

  if (header.major_number != std::get<0> (release_number ()))
  {
    throw std::runtime_error (
      "Incompatible release number");
  }
    
  if (header.trie_segment.first != 0 &&
      header.trie_segment.second == 0)
  {
    throw std::runtime_error (
      "Invalid empty trie segment in header");
  }
    
  /*
   * Read score table segment
   */
  if (header.score_table_segment.first)
  {
    fin.seekg (header.score_table_segment.first, std::ios_base::beg);
    
    if (!header.score_table_segment.second)
    {
      throw std::runtime_error (
	"Invalid empty score table length");
    }
    
    serialised_score_table.resize (header.score_table_segment.second);

    fin.read (
      reinterpret_cast<char *> (serialised_score_table.data ()),
      serialised_score_table.size ());
  }

  /*
   * Read trie segment
   */
  fin.seekg (header.trie_segment.first, std::ios_base::beg);
  
  serialised_trie.resize (header.trie_segment.second);

  fin.read (
    reinterpret_cast<char *> (serialised_trie.data ()),
    serialised_trie.size ());
  
  return from_memory (std::move (serialised_trie),
		      std::move (serialised_score_table));
}
  
template<typename Parameters>
void Store<Parameters>::write (const std::string &path) const
{
  std::ofstream fout (path, std::ios_base::out |
                            std::ios_base::binary |
		            std::ios_base::trunc);
  /*
   * Construct header
   */
  const auto header = [&]
  {
    Header<Parameters> result;
    
    const auto header_size =
      serialised_header_size<Parameters> ();
    
    if (!m_serialised_score_table.empty ())
    {
      result.score_table_segment = std::make_pair (
        header_size,
        m_serialised_score_table.size ());
    }

    result.trie_segment = std::make_pair (
      header_size + m_serialised_score_table.size (),
      m_serialised_trie.size ());

    return result;
  } ();
  
  put_header (fout, header);

  /*
   * Write score table and trie
   */
  if (!m_serialised_score_table.empty ())
  {
    fout.write (
      reinterpret_cast<const char *> (
	m_serialised_score_table.data ()),
      m_serialised_score_table.size ());
  }

  fout.write (
    reinterpret_cast<const char *> (
      m_serialised_trie.data ()),
    m_serialised_trie.size ());

  if (!fout)
  {
    throw std::runtime_error ("Error writing to file");
  }
}

template<typename Parameters>
auto Store<Parameters>::trie_data () const
  -> std::pair<const std::uint8_t*,
               const std::uint8_t*>
{
  return m_serialised_trie.empty () ?
     std::make_pair (nullptr, nullptr)
   : std::make_pair (m_serialised_trie.data (),
 		     &(m_serialised_trie.back ()) + 1);
}

template<typename Parameters>
auto Store<Parameters>::score_table_data () const
  -> std::pair<const std::uint8_t*,
               const std::uint8_t*>
{
  return m_serialised_score_table.empty () ?
      std::make_pair (nullptr, nullptr)
    : std::make_pair (m_serialised_score_table.data (),
  		     &(m_serialised_score_table.back ()) + 1);
}
  
} // namespace detail
} // namespace ordered_trie

#endif
