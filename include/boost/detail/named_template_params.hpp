// (C) Copyright Jeremy Siek 2001. Permission to copy, use, modify,
// sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.

// Revision History:
//
// 27 June 2001  Jeremy Siek
//      Simplified the mechanism.
// 08 Mar 2001   Jeremy Siek
//      Initial version.

#ifndef BOOST_DETAIL_NAMED_TEMPLATE_PARAMS_HPP
#define BOOST_DETAIL_NAMED_TEMPLATE_PARAMS_HPP

#include <boost/type_traits/conversion_traits.hpp>
#include <boost/type_traits/same_traits.hpp>
#include <boost/type_traits/composite_traits.hpp> // for is_reference
#include <boost/pending/ct_if.hpp>
#if defined(__BORLANDC__)
#include <boost/type_traits/ice.hpp>
#endif

/* 
   template <class List, class Tag>
   struct find_param;
   
   This searches through a compile-time associative "List" of types
   to find the value (which is a type) associated with key specified
   by "Tag". The list is a lisp-style cons list using std::pair.
   The first_type of each pair must be a type that has a
   "type" and "tag" member, where "type" is the value and "tag"
   is the key that will match with the "Tag". The *_is classes
   in boost/iterator_adaptors.hpp (like value_type_is) are
   example of such types.
   

   struct named_template_param_base;
   template <class Arg, class Tag> struct wrap_param;

   The named_template_param_base can be used to distinguish
   normal arguments from named arguments, the *_is classes
   inherit from named_template_param_base. The wrap_param
   class returns named arguments unchanged and returns
   normal arguments wrapped with the param_is type so
   that it will fit into the associative list.

   
   template <class Arg, class DefaultGen, class Info>
   class resolve_default;
   
   This class figures out what the argument type of a parameter is
   based on the argument passed in (which might be the
   default_argument type), a default generator, and an Info type,

   The DefaultGen type should have a single member class template
   named "bind" with one template parameter for the Info type. The
   bind class should have a single typedef named "type" that produces
   the default. See boost/iterator_adaptors.hpp for example usage.

  The Info type is any type that might be useful in computing the
  default inside of the "bind" member class.

*/


namespace boost {

  // To differentiate an unnamed parameter from a traits generator
  // we use is_convertible<X, iter_traits_gen_base>.
  struct named_template_param_base { };

  namespace detail {
    
    struct default_argument { };

    struct dummy_default_gen {
      template <class Info>
      struct bind {
        typedef default_argument type;
      };
    };

    // Specializations of this class template are used as a workaround
    // for MSVC. For other compilers this unspecialized version is
    // used (which doesn't really do anything).
    template <class Gen> struct default_generator {
      typedef Gen type;
    };

    template <class T> struct is_default { 
      enum { value = false };  
      typedef type_traits::no_type type;
    };
    template <> struct is_default<default_argument> { 
      enum { value = true }; 
      typedef type_traits::yes_type type;
    };

    struct choose_default {
      template <class Arg, class DefaultGen, class Info>
      struct bind {
        typedef typename default_generator<DefaultGen>::type Gen;
        typedef typename Gen::template bind<Info>::type type;
      };
    };
    struct choose_arg {
      template <class Arg, class DefaultGen, class Info>
      struct bind {
        typedef Arg type;
      };
    };

#if defined(__BORLANDC__)
    template <class UseDefault>
    struct choose_arg_or_default { typedef choose_arg type; };
    template <>
    struct choose_arg_or_default<type_traits::yes_type> {
      typedef choose_default type;
    };
#else
    template <bool UseDefault>
    struct choose_arg_or_default { typedef choose_arg type; };
    template <>
    struct choose_arg_or_default<true> {
      typedef choose_default type;
    };
#endif
    
    template <class Arg, class DefaultGen, class Info>
    class resolve_default {
#if defined(__BORLANDC__)
      typedef typename choose_arg_or_default<typename is_default<Arg>::type>::type Selector;
#else
      // This usually works for Borland, but I'm seeing weird errors in
      // iterator_adaptor_test.cpp when using this method.
      enum { is_def = is_default<Arg>::value };
      typedef typename choose_arg_or_default<is_def>::type Selector;
#endif
    public:
      typedef typename Selector
        ::template bind<Arg, DefaultGen, Info>::type type;
    };

    template <class X>
    struct is_named_param_list {
      enum { value  = is_convertible<X, named_template_param_base>::value };
    };
    
    struct choose_named_params {
      template <class Prev> struct bind { typedef Prev type; };
    };
    struct choose_default_arg {
      template <class Prev> struct bind { 
        typedef detail::default_argument type;
      };
    };

    template <bool Named> struct choose_default_dispatch { };
    template <> struct choose_default_dispatch<true> {
      typedef choose_named_params type;
    };
    template <> struct choose_default_dispatch<false> {
      typedef choose_default_arg type;
    };


    template <class PreviousArg>
    struct choose_default_argument {
      enum { is_named = is_named_param_list<PreviousArg>::value };
      typedef typename choose_default_dispatch<is_named>::type Selector;
      typedef typename Selector::template bind<PreviousArg>::type type;
    };

  } // namespace detail

  struct list_end_type { };

  namespace detail {
    template <class List> 
    struct find_param_impl {
      template <class Tag> class bind {
        typedef typename List::first_type Param;
        typedef typename List::second_type Rest;
        typedef typename find_param_impl<Rest>::template bind<Tag>::type 
          RestType;
        enum { is_same_tag = is_same<typename Param::tag, Tag>::value };
      public:
        typedef typename ct_if<is_same_tag,
          typename Param::type, RestType>::type type;
      };
    };
    template <> 
    struct find_param_impl<list_end_type> {
      template <class Tag> struct bind {
        typedef default_argument type;
      };
    };

    template <class T, class Tag>
    struct param_is {
      typedef T type;
      typedef Tag tag;
    };

  } //namespace detail

  template <class List, class Tag> struct find_param {
    typedef typename detail::find_param_impl<List>::template bind<Tag>::type 
      type;
  };

  template <class T, class Tag>
  struct wrap_param {
    enum { is_named_param = 
           is_convertible<T, named_template_param_base>::value };
    typedef typename ct_if<is_named_param, T, 
      detail::param_is<T, Tag> >::type type;
  };

} // namespace boost

#endif // BOOST_DETAIL_NAMED_TEMPLATE_PARAMS_HPP
