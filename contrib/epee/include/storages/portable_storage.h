// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 



#pragma once 

#include <boost/mpl/contains.hpp>
#include "misc_language.h"
#include "portable_storage_base.h"
#include "portable_storage_to_bin.h"
#include "portable_storage_from_bin.h"
#include "portable_storage_to_json.h"
#include "portable_storage_from_json.h"
#include "portable_storage_val_converters.h"

namespace epee
{
  namespace serialization
  {
    /************************************************************************/
    /*                                                                      */
    /************************************************************************/
    template<typename t_section>
    class portable_storage_base
    {
    public:
      //typedef epee::serialization::hsection hsection;
      using use_descriptions = std::false_type;
      typedef t_section* hsection;
      typedef epee::serialization::harray  harray;
      typedef storage_entry meta_entry;

      portable_storage_base
      (){}
      virtual ~portable_storage_base
      (){}
      hsection   open_section(const std::string& section_name,  hsection hparent_section, bool create_if_notexist = false);
      template<class t_value>
      bool       get_value(const std::string& value_name, t_value& val, hsection hparent_section);
      bool       get_value(const std::string& value_name, storage_entry& val, hsection hparent_section);
      template<class t_value>
      bool       set_value(const std::string& value_name, const t_value& target, hsection hparent_section);

      //serial access for arrays of values --------------------------------------
      //values
      template<class t_value>
      harray        get_first_value(const std::string& value_name, t_value& target, hsection hparent_section);
      template<class t_value>
      bool          get_next_value(harray hval_array, t_value& target);
      template<class t_value>
      harray        insert_first_value(const std::string& value_name, const t_value& target, hsection hparent_section);
      template<class t_value>
      bool          insert_next_value(harray hval_array, const t_value& target);
      //sections
      harray        get_first_section(const std::string& pSectionName, hsection& h_child_section, hsection hparent_section);
      bool          get_next_section(harray hSecArray, hsection& h_child_section);
      harray        insert_first_section(const std::string& pSectionName, hsection& hinserted_childsection, hsection hparent_section);
      bool          insert_next_section(harray hSecArray, hsection& hinserted_childsection);
      //------------------------------------------------------------------------
      //delete entry (section, value or array)
      bool        delete_entry(const std::string& pentry_name, hsection hparent_section = nullptr);      
      //-------------------------------------------------------------------------------
      bool		store_to_binary(binarybuffer& target);
      bool		load_from_binary(const binarybuffer& target);
      template<class trace_policy>
      bool		  dump_as_xml(std::string& targetObj, const std::string& root_name = "");
      bool		  dump_as_json(std::string& targetObj, size_t indent = 0/*, end_of_line_t eol = eol_crlf*/);
      bool		  load_from_json(const std::string& source);
      void      set_entry_description(hsection hparent_section, const std::string& name, const std::string& description) {}

      template<typename cb_t>
      bool enum_entries(hsection hparent_section, cb_t cb);
    protected:
      section m_root;
      hsection	get_root_section() {return &m_root;}
      storage_entry* find_storage_entry(const std::string& pentry_name, hsection psection);
      template<class entry_type>
      storage_entry* insert_new_entry_get_storage_entry(const std::string& pentry_name, hsection psection, const entry_type& entry);

      hsection    insert_new_section(const std::string& pentry_name, hsection psection);

#pragma pack(push)
#pragma pack(1)
      struct storage_block_header
      {
        uint32_t m_signature_a;
        uint32_t m_signature_b;
        uint8_t  m_ver;
      };
#pragma pack(pop)
    };


    template<typename t_section>
    bool portable_storage_base<t_section>::dump_as_json(std::string& buff, size_t indent /* = 0 *//*, end_of_line_t eol *//* = eol_crlf */)
    {
      TRY_ENTRY();
      std::stringstream ss;
      epee::serialization::recursive_visitor<strategy_json>::dump_as_(ss, m_root, indent/*, eol*/);
      buff = ss.str();
      return true;
      CATCH_ENTRY("portable_storage_base<t_section>::dump_as_json", false)
    }

    template<typename t_section>
    bool portable_storage_base<t_section>::load_from_json(const std::string& source)
    {
      TRY_ENTRY();
      return json::load_from_json(source, *this);
      CATCH_ENTRY("portable_storage_base<t_section>::load_from_json", false)
    }

    template<typename t_section>
    template<class trace_policy>
    bool portable_storage_base<t_section>::dump_as_xml(std::string& targetObj, const std::string& root_name)
    {
      return false;//TODO: don't think i ever again will use xml - ambiguous and "overtagged" format
    }

    template<typename t_section>
    bool portable_storage_base<t_section>::store_to_binary(binarybuffer& target)
    {
      TRY_ENTRY();
      std::stringstream ss;
      storage_block_header sbh = AUTO_VAL_INIT(sbh);
      sbh.m_signature_a = PORTABLE_STORAGE_SIGNATUREA;
      sbh.m_signature_b = PORTABLE_STORAGE_SIGNATUREB;
      sbh.m_ver = PORTABLE_STORAGE_FORMAT_VER;
      ss.write((const char*)&sbh, sizeof(storage_block_header));
      pack_entry_to_buff(ss, m_root);
      target = ss.str();
      return true;
      CATCH_ENTRY("portable_storage_base<t_section>::store_to_binary", false)
    }
    template<typename t_section>
    bool portable_storage_base<t_section>::load_from_binary(const binarybuffer& source)
    {
      m_root.m_entries.clear();
      if(source.size() < sizeof(storage_block_header))
      {
        LOG_ERROR("portable_storage: wrong binary format, packet size = " << source.size() << " less than expected sizeof(storage_block_header)=" << sizeof(storage_block_header));
        return false;
      }
      storage_block_header* pbuff = (storage_block_header*)source.data();
      if(pbuff->m_signature_a != PORTABLE_STORAGE_SIGNATUREA || 
        pbuff->m_signature_b != PORTABLE_STORAGE_SIGNATUREB 
        )
      {
        LOG_PRINT_RED_L0("portable_storage: wrong binary format - signature missmatch");
        return false;
      }
      if(pbuff->m_ver != PORTABLE_STORAGE_FORMAT_VER)
      {
        LOG_ERROR("portable_storage: wrong binary format - unknown format ver = " << pbuff->m_ver);
        return false;
      }
      TRY_ENTRY();
      throwable_buffer_reader buf_reader(source.data()+sizeof(storage_block_header), source.size()-sizeof(storage_block_header));
      buf_reader.read(m_root);
      return true;//TODO:
      CATCH_ENTRY("portable_storage_base<t_section>::load_from_binary", false);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    typename portable_storage_base<t_section>::hsection portable_storage_base<t_section>::open_section(const std::string& section_name,  hsection hparent_section, bool create_if_notexist)
    {
      TRY_ENTRY();
      hparent_section = hparent_section ? hparent_section:&m_root;
      storage_entry* pentry = find_storage_entry(section_name, hparent_section);
      if(!pentry)
      {
        if(!create_if_notexist)
          return nullptr;
        return insert_new_section(section_name, hparent_section);
      }
      CHECK_AND_ASSERT(pentry , nullptr);
      //check that section_entry we find is real "CSSection"
      if(pentry->type() != typeid(section))
      {
        if(create_if_notexist)
          *pentry = storage_entry(section());//replace
        else
          return nullptr;
      }
      return &boost::get<section>(*pentry);
      CATCH_ENTRY("portable_storage_base<t_section>::open_section", nullptr);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<class to_type>
    struct get_value_visitor: boost::static_visitor<void>
    {
      to_type& m_target;
      get_value_visitor(to_type& target):m_target(target){}
      template<class from_type>
      void operator()(const from_type& v){convert_t(v, m_target);}
    };

    template<typename t_section>
    template<class t_value>
    bool portable_storage_base<t_section>::get_value(const std::string& value_name, t_value& val, hsection hparent_section)
    {
      BOOST_MPL_ASSERT(( boost::mpl::contains<storage_entry::types, t_value> )); 
      //TRY_ENTRY();
      if(!hparent_section) hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(value_name, hparent_section);
      if(!pentry)
        return false;

      get_value_visitor<t_value> gvv(val);
      boost::apply_visitor(gvv, *pentry);
      return true;
      //CATCH_ENTRY("portable_storage_base<t_section>::template<>get_value", false);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    bool portable_storage_base<t_section>::get_value(const std::string& value_name, storage_entry& val, hsection hparent_section)
    {
      //TRY_ENTRY();
      if(!hparent_section) hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(value_name, hparent_section);
      if(!pentry)
        return false;

      val = *pentry;
      return true;
      //CATCH_ENTRY("portable_storage_base<t_section>::template<>get_value", false);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    template<class t_value>
    bool portable_storage_base<t_section>::set_value(const std::string& value_name, const t_value& v, hsection hparent_section)        
    {
      BOOST_MPL_ASSERT(( boost::mpl::contains<boost::mpl::push_front<storage_entry::types, storage_entry>::type, t_value> )); 
      TRY_ENTRY();
      if(!hparent_section)
        hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(value_name, hparent_section);
      if(!pentry)
      {
        pentry = insert_new_entry_get_storage_entry(value_name, hparent_section, v);
        if(!pentry)
          return false;
        return true;
      }
      *pentry = storage_entry(v);
      return true;
      CATCH_ENTRY("portable_storage_base<t_section>::template<>set_value", false);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    storage_entry* portable_storage_base<t_section>::find_storage_entry(const std::string& pentry_name, hsection psection)
    {
      TRY_ENTRY();
      CHECK_AND_ASSERT(psection, nullptr);
      auto it = psection->m_entries.find(pentry_name);
      if(it == psection->m_entries.end())
        return nullptr;

      return &it->second;
      CATCH_ENTRY("portable_storage_base<t_section>::find_storage_entry", nullptr);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    template<class entry_type>
    storage_entry* portable_storage_base<t_section>::insert_new_entry_get_storage_entry(const std::string& pentry_name, hsection psection, const entry_type& entry)
    {
      TRY_ENTRY();
      CHECK_AND_ASSERT(psection, nullptr);
      auto ins_res = psection->m_entries.insert(std::pair<std::string, storage_entry>(pentry_name, entry));
      return &ins_res.first->second;
      CATCH_ENTRY("portable_storage_base<t_section>::insert_new_entry_get_storage_entry", nullptr);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    typename portable_storage_base<t_section>::hsection portable_storage_base<t_section>::insert_new_section(const std::string& pentry_name, hsection psection)
    {
      TRY_ENTRY();
      storage_entry* pse = insert_new_entry_get_storage_entry(pentry_name, psection, section());
      if(!pse) return nullptr;
      return &boost::get<section>(*pse);
      CATCH_ENTRY("portable_storage_base<t_section>::insert_new_section", nullptr);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<class to_type>
    struct get_first_value_visitor: boost::static_visitor<bool>
    {
      to_type& m_target;
      get_first_value_visitor(to_type& target):m_target(target){}
      template<class from_type>
      bool operator()(const array_entry_t<from_type>& a)
      {
        const from_type* pv = a.get_first_val();
        if(!pv)
          return false;
        convert_t(*pv, m_target);
        return true;
      }
    };
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    template<class t_value>
    harray portable_storage_base<t_section>::get_first_value(const std::string& value_name, t_value& target, hsection hparent_section)
    {
      BOOST_MPL_ASSERT(( boost::mpl::contains<storage_entry::types, t_value> )); 
      //TRY_ENTRY();
      if(!hparent_section) hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(value_name, hparent_section);
      if(!pentry)
        return nullptr;
      if(pentry->type() != typeid(array_entry))
        return nullptr;
      array_entry& ar_entry = boost::get<array_entry>(*pentry);
      
      get_first_value_visitor<t_value> gfv(target);
      if(!boost::apply_visitor(gfv, ar_entry))
        return nullptr;
      return &ar_entry;
      //CATCH_ENTRY("portable_storage_base<t_section>::get_first_value", nullptr);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<class to_type>
    struct get_next_value_visitor: boost::static_visitor<bool>
    {
      to_type& m_target;
      get_next_value_visitor(to_type& target):m_target(target){}
      template<class from_type>
      bool operator()(const array_entry_t<from_type>& a)
      {
        //TODO: optimize code here: work without get_next_val function
        const from_type* pv = a.get_next_val();
        if(!pv)
          return false;
        convert_t(*pv, m_target);
        return true;
      }
    };
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    template<class t_value>
    bool portable_storage_base<t_section>::get_next_value(harray hval_array, t_value& target)
    {
      BOOST_MPL_ASSERT(( boost::mpl::contains<storage_entry::types, t_value> )); 
      //TRY_ENTRY();
      CHECK_AND_ASSERT(hval_array, false);
      array_entry& ar_entry = *hval_array;
      get_next_value_visitor<t_value> gnv(target);
      if(!boost::apply_visitor(gnv, ar_entry))
        return false;
      return true;
      //CATCH_ENTRY("portable_storage_base<t_section>::get_next_value", false);
    } 
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    template<class t_value>
    harray portable_storage_base<t_section>::insert_first_value(const std::string& value_name, const t_value& target, hsection hparent_section)
    {
      TRY_ENTRY();
      if(!hparent_section) hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(value_name, hparent_section);
      if(!pentry)
      {
        pentry = insert_new_entry_get_storage_entry(value_name, hparent_section, array_entry(array_entry_t<t_value>()));
        if(!pentry)
          return nullptr;
      }
      if(pentry->type() != typeid(array_entry))
        *pentry = storage_entry(array_entry(array_entry_t<t_value>()));

      array_entry& arr = boost::get<array_entry>(*pentry);
      if(arr.type() != typeid(array_entry_t<t_value>))
        arr = array_entry(array_entry_t<t_value>());

      array_entry_t<t_value>& arr_typed = boost::get<array_entry_t<t_value> >(arr);
      arr_typed.insert_first_val(target);
      return &arr;
      CATCH_ENTRY("portable_storage_base<t_section>::insert_first_value", nullptr);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    template<typename cb_t>
    bool portable_storage_base<t_section>::enum_entries(hsection hparent_section, cb_t cb)
    {
      TRY_ENTRY();
      if (!hparent_section) hparent_section = &m_root;
      for (const auto& e : hparent_section->m_entries)
      {
        if (!cb(e.first, e.second))
          break;
      }
      return true;
      CATCH_ENTRY("portable_storage_base<t_section>::enum_entries", false);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    template<class t_value>
    bool portable_storage_base<t_section>::insert_next_value(harray hval_array, const t_value& target)
    {
      TRY_ENTRY();
      CHECK_AND_ASSERT(hval_array, false);

      CHECK_AND_ASSERT_MES(hval_array->type() == typeid(array_entry_t<t_value>), 
        false, "unexpected type in insert_next_value: " << typeid(array_entry_t<t_value>).name());

      array_entry_t<t_value>& arr_typed = boost::get<array_entry_t<t_value> >(*hval_array);
      arr_typed.insert_next_value(target);
      return true;
      CATCH_ENTRY("portable_storage_base<t_section>::insert_next_value", false);
    }
    //---------------------------------------------------------------------------------------------------------------
    //sections
    template<typename t_section>
    harray portable_storage_base<t_section>::get_first_section(const std::string& sec_name, hsection& h_child_section, hsection hparent_section)
    {
      TRY_ENTRY();
      if(!hparent_section) hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(sec_name, hparent_section);
      if(!pentry)
        return nullptr;
      if(pentry->type() != typeid(array_entry))
        return nullptr;
      array_entry& ar_entry = boost::get<array_entry>(*pentry);
      if(ar_entry.type() != typeid(array_entry_t<section>))
        return nullptr;
      array_entry_t<section>& sec_array = boost::get<array_entry_t<section>>(ar_entry);
      section* psec = sec_array.get_first_val();
      if(!psec)
        return nullptr;
      h_child_section = psec;
      return &ar_entry;
      CATCH_ENTRY("portable_storage_base<t_section>::get_first_section", nullptr);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    bool portable_storage_base<t_section>::get_next_section(harray hsec_array, hsection& h_child_section)
    {
      TRY_ENTRY();
      CHECK_AND_ASSERT(hsec_array, false);
      if(hsec_array->type() != typeid(array_entry_t<section>))
        return false;
      array_entry_t<section>& sec_array = boost::get<array_entry_t<section>>(*hsec_array);
      h_child_section = sec_array.get_next_val();
      if(!h_child_section)
        return false;
      return true;
      CATCH_ENTRY("portable_storage_base<t_section>::get_next_section", false);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    harray portable_storage_base<t_section>::insert_first_section(const std::string& sec_name, hsection& hinserted_childsection, hsection hparent_section)
    {
      TRY_ENTRY();
      if(!hparent_section) hparent_section = &m_root;
      storage_entry* pentry = find_storage_entry(sec_name, hparent_section);
      if(!pentry)
      {
        pentry = insert_new_entry_get_storage_entry(sec_name, hparent_section, array_entry(array_entry_t<section>()));
        if(!pentry)
          return nullptr;
      }
      if(pentry->type() != typeid(array_entry))
        *pentry = storage_entry(array_entry(array_entry_t<section>()));

      array_entry& ar_entry = boost::get<array_entry>(*pentry);
      if(ar_entry.type() != typeid(array_entry_t<section>))
        ar_entry = array_entry(array_entry_t<section>());

      array_entry_t<section>& sec_array = boost::get<array_entry_t<section>>(ar_entry);
      hinserted_childsection = &sec_array.insert_first_val(section());
      return &ar_entry;
      CATCH_ENTRY("portable_storage_base<t_section>::insert_first_section", nullptr);
    }
    //---------------------------------------------------------------------------------------------------------------
    template<typename t_section>
    bool portable_storage_base<t_section>::insert_next_section(harray hsec_array, hsection& hinserted_childsection)
    {
      TRY_ENTRY();
      CHECK_AND_ASSERT(hsec_array, false);
      CHECK_AND_ASSERT_MES(hsec_array->type() == typeid(array_entry_t<section>), 
        false, "unexpected type(not 'section') in insert_next_section, type: " << hsec_array->type().name());

      array_entry_t<section>& sec_array = boost::get<array_entry_t<section>>(*hsec_array);
      hinserted_childsection = &sec_array.insert_next_value(section());
      return true;
      CATCH_ENTRY("portable_storage_base<t_section>::insert_next_section", false);
    }
    //---------------------------------------------------------------------------------------------------------------    
    typedef portable_storage_base<section> portable_storage;
  }
}
