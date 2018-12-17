#include <fc/log/logger_config.hpp>

#include <iostream>
#include <string>
#include <unordered_map>

#include <fc/filesystem.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>

#include <fc/log/appender.hpp>
#include <fc/log/console_appender.hpp>

#ifndef FCLITE
#include <fc/log/gelf_appender.hpp>
#endif

namespace fc {
   extern std::unordered_map<std::string,logger>& get_logger_map();
   extern std::unordered_map<std::string,appender::ptr>& get_appender_map();
   logger_config& logger_config::add_appender( const string& s ) { appenders.push_back(s); return *this; }

   void configure_logging( const fc::path& lc )
   {
      configure_logging( fc::json::from_file<logging_config>(lc) );
   }
   bool configure_logging( const logging_config& cfg )
   {
      try {
      static bool reg_console_appender = appender::register_appender<console_appender>( "console" );
#ifndef FCLITE
      static bool reg_gelf_appender = appender::register_appender<gelf_appender>( "gelf" );
#endif
      get_logger_map().clear();
      get_appender_map().clear();

      //slog( "\n%s", fc::json::to_pretty_string(cfg).c_str() );
      for( size_t i = 0; i < cfg.appenders.size(); ++i ) {
         appender::create( cfg.appenders[i].name, cfg.appenders[i].type, cfg.appenders[i].args );
        // TODO... process enabled
      }
      for( size_t i = 0; i < cfg.loggers.size(); ++i ) {
         auto lgr = logger::get( cfg.loggers[i].name );

         // TODO: finish configure logger here...
         if( cfg.loggers[i].parent.has_value() ) {
            lgr.set_parent( logger::get( *cfg.loggers[i].parent ) );
         }
         lgr.set_name(cfg.loggers[i].name);
         if( cfg.loggers[i].level.has_value() ) lgr.set_log_level( *cfg.loggers[i].level );


         for( auto a = cfg.loggers[i].appenders.begin(); a != cfg.loggers[i].appenders.end(); ++a ){
            auto ap = appender::get( *a );
            if( ap ) { lgr.add_appender(ap); }
         }
      }
#ifndef FCLITE
      return reg_console_appender || reg_gelf_appender;
#else
      return reg_console_appender;
#endif
      } catch ( exception& e )
      {
         std::cerr<<e.to_detail_string()<<"\n";
      }
      return false;
   }

   logging_config logging_config::default_config() {
      //slog( "default cfg" );
      logging_config cfg;

     variants  c;
               c.push_back(  mutable_variant_object( "level","debug")("color", "green") );
               c.push_back(  mutable_variant_object( "level","warn")("color", "brown") );
               c.push_back(  mutable_variant_object( "level","error")("color", "red") );

      cfg.appenders.push_back(
             appender_config( "stderr", "console",
                 mutable_variant_object()
                     ( "stream","std_error")
                     ( "level_colors", c )
                 ) );
      cfg.appenders.push_back(
             appender_config( "stdout", "console",
                 mutable_variant_object()
                     ( "stream","std_out")
                     ( "level_colors", c )
                 ) );

      logger_config dlc;
      dlc.name = "default";
      dlc.level = log_level::debug;
      dlc.appenders.push_back("stderr");
      cfg.loggers.push_back( dlc );
      return cfg;
   }

   static thread_local std::string thread_name;
   void set_thread_name( const string& name ) {
      thread_name = name;
   }
   const string& get_thread_name() {
      static int thread_count = 0;
      if( thread_name.empty() )
         thread_name = string("thread-")+fc::to_string(thread_count++);
      return thread_name;
   }
}
