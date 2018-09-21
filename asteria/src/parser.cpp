// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "parser.hpp"
#include "token_stream.hpp"
#include "token.hpp"
#include "statement.hpp"
#include "block.hpp"
#include "xpnode.hpp"
#include "expression.hpp"
#include "utilities.hpp"

namespace Asteria {

Parser::~Parser()
  {
  }

namespace {

  inline Parser_result do_make_parser_result(const Token_stream &toks_io, Parser_result::Error error)
    {
      const auto qtok = toks_io.peek_opt();
      if(!qtok) {
        return Parser_result(0, 0, 0, error);
      }
      return Parser_result(qtok->get_line(), qtok->get_offset(), qtok->get_length(), error);
    }

  bool do_match_keyword(Token_stream &toks_io, Token::Keyword keyword)
    {
      const auto qtok = toks_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_keyword>();
      if(!qalt) {
        return false;
      }
      if(qalt->keyword != keyword) {
        return false;
      }
      toks_io.shift();
      return true;
    }

  bool do_match_punctuator(Token_stream &toks_io, Token::Punctuator punct)
    {
      const auto qtok = toks_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_punctuator>();
      if(!qalt) {
        return false;
      }
      if(qalt->punct != punct) {
        return false;
      }
      toks_io.shift();
      return true;
    }

  bool do_accept_identifier(String &name_out, Token_stream &toks_io)
    {
      const auto qtok = toks_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_identifier>();
      if(!qalt) {
        return false;
      }
      name_out = qalt->name;
      toks_io.shift();
      return true;
    }

  bool do_match_string_literal(String &value_out, Token_stream &toks_io)
    {
      const auto qtok = toks_io.peek_opt();
      if(!qtok) {
        return false;
      }
      const auto qalt = qtok->opt<Token::S_string_literal>();
      if(!qalt) {
        return false;
      }
      value_out = qalt->value;
      toks_io.shift();
      return true;
    }

  bool do_accept_export_directive(Statement &stmt_out, Token_stream &toks_io)
    {
      // export-directive ::=
      //   "export" identifier ";"
      if(do_match_keyword(toks_io, Token::keyword_export) == false) {
        return false;
      }
      String name;
      if(do_accept_identifier(name, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_export stmt_c = { std::move(name) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_import_directive(Statement &stmt_out, Token_stream &toks_io)
    {
      // import-directive ::=
      //   "import" string-literal ";"
      if(do_match_keyword(toks_io, Token::keyword_import) == false) {
        return false;
      }
      String path;
      if(do_match_string_literal(path, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_string_literal_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_import stmt_c = { std::move(path) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  template<typename SinkT>
    extern bool do_accept_statement(SinkT &sink_out, Token_stream &toks_io);

  bool do_accept_block(Block &block_out, Token_stream &toks_io)
    {
      // block ::=
      //   "{" statement-list-opt "}"
      // statement-list-opt ::=
      //   statement-list | ""
      // statement-list ::=
      //   statement statement-list-opt
      if(do_match_punctuator(toks_io, Token::punctuator_brace_op) == false) {
        return false;
      }
      Vector<Statement> stmts;
      for(;;) {
        Statement stmt;
        if(do_accept_statement(stmt, toks_io) == false) {
          break;
        }
        stmts.emplace_back(std::move(stmt));
      }
      if(do_match_punctuator(toks_io, Token::punctuator_brace_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_brace_or_statement_expected);
      }
      block_out = std::move(stmts);
      return true;
    }
  bool do_accept_block(Statement &stmt_out, Token_stream &toks_io)
    {
      Block block;
      if(do_accept_block(block, toks_io) == false) {
        return false;
      }
      Statement::S_block stmt_c = { std::move(block) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_null_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // null-statement ::=
      //   ";"
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        return false;
      }
      Statement::S_null stmt_c = { };
      stmt_out = std::move(stmt_c);
      return true;
    }

  void do_tell_source_location(String &file_out, Uint64 &line_out, const Token_stream &toks_io)
    {
      const auto qtok = toks_io.peek_opt();
      if(!qtok) {
        return;
      }
      file_out = qtok->get_file();
      line_out = qtok->get_line();
    }

  bool do_accept_identifier_list(Vector<String> &names_out, Token_stream &toks_io)
    {
      // identifier-list-opt ::=
      //   identifier-list | ""
      // identifier-list ::=
      //   identifier ( "," identifier-list | "" )
      String name;
      if(do_accept_identifier(name, toks_io) == false) {
        return false;
      }
      names_out.emplace_back(std::move(name));
      while(do_match_punctuator(toks_io, Token::punctuator_comma)) {
        if(do_accept_identifier(name, toks_io) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
        }
        names_out.emplace_back(std::move(name));
      }
      return true;
    }

  bool do_accept_expression(Expression &expr_out, Token_stream &toks_io)
    {
      // TODO
      auto qk = toks_io.peek_opt();
      if(!qk) {
        return false;
      }
      if(qk->index() == Token::index_keyword) {
        return false;
      }
      if(qk->index() == Token::index_punctuator) {
        return false;
      }
      toks_io.shift();
      return true;
    }

  bool do_accept_variable_definition(Statement &stmt_out, Token_stream &toks_io)
    {
      // variable-definition ::=
      //   "var" identifier equal-initailizer-opt ";"
      // equal-initializer-opt ::=
      //   equal-initializer | ""
      // equal-initializer ::=
      //   "=" expression
      if(do_match_keyword(toks_io, Token::keyword_var) == false) {
        return false;
      }
      String name;
      if(do_accept_identifier(name, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
      }
      Expression init;
      if(do_match_punctuator(toks_io, Token::punctuator_assign)) {
        if(do_accept_expression(init, toks_io) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
        }
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_var_def stmt_c = { std::move(name), false, std::move(init) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_immutable_variable_definition(Statement &stmt_out, Token_stream &toks_io)
    {
      // immutable-variable-definition ::=
      //   "const" identifier equal-initailizer ";"
      // equal-initializer ::=
      //   "=" expression
      if(do_match_keyword(toks_io, Token::keyword_const) == false) {
        return false;
      }
      String name;
      if(do_accept_identifier(name, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_assign) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_equals_sign_expected);
      }
      Expression init;
      if(do_accept_expression(init, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_var_def stmt_c = { std::move(name), true, std::move(init) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_function_definition(Statement &stmt_out, Token_stream &toks_io)
    {
      // Copy these parameters before reading from the stream which is destructive.
      String file;
      Uint64 line = 0;
      do_tell_source_location(file, line, toks_io);
      // function-definition ::=
      //   "func" identifier "(" identifier-list-opt ")" statement
      if(do_match_keyword(toks_io, Token::keyword_func) == false) {
        return false;
      }
      String name;
      if(do_accept_identifier(name, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      Vector<String> params;
      do_accept_identifier_list(params, toks_io);
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Block body;
      if(do_accept_statement(body, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      Statement::S_func_def stmt_c = { std::move(file), line, std::move(name), std::move(params), std::move(body) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_expression_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // expression-statement ::=
      //   expression ";"
      Expression expr;
      if(do_accept_expression(expr, toks_io) == false) {
        return false;
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_expr stmt_c = { std::move(expr) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_if_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // if-statement ::=
      //   "if" "(" expression ")" statement ( "else" statement | "" )
      if(do_match_keyword(toks_io, Token::keyword_if) == false) {
        return false;
      }
      Expression cond;
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      if(do_accept_expression(cond, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Block branch_true;
      if(do_accept_statement(branch_true, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      Block branch_false;
      if(do_match_keyword(toks_io, Token::keyword_else)) {
        if(do_accept_statement(branch_false, toks_io) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
        }
      }
      Statement::S_if stmt_c = { std::move(cond), std::move(branch_true), std::move(branch_false) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_switch_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // switch-statement ::=
      //   "switch" "(" expression ")" switch-block
      // switch-block ::=
      //   "{" swtich-clause-list-opt "}"
      // switch-clause-list-opt ::=
      //   switch-clause-list | ""
      // switch-clause-list ::=
      //   ( "case" expression | "default" ) ":" statement-list-opt switch-clause-list-opt
      if(do_match_keyword(toks_io, Token::keyword_switch) == false) {
        return false;
      }
      Expression ctrl;
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      if(do_accept_expression(ctrl, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Bivector<Expression, Block> clauses;
      if(do_match_punctuator(toks_io, Token::punctuator_brace_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_brace_expected);
      }
      for(;;) {
        Expression cond;
        if(do_match_keyword(toks_io, Token::keyword_default) == false) {
          if(do_match_keyword(toks_io, Token::keyword_case) == false) {
            break;
          }
          if(do_accept_expression(cond, toks_io) == false) {
            throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
          }
        }
        if(do_match_punctuator(toks_io, Token::punctuator_colon) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_colon_expected);
        }
        Vector<Statement> stmts;
        for(;;) {
          Statement stmt;
          if(do_accept_statement(stmt, toks_io) == false) {
            break;
          }
          stmts.emplace_back(std::move(stmt));
        }
        clauses.emplace_back(std::move(cond), std::move(stmts));
      }
      if(do_match_punctuator(toks_io, Token::punctuator_brace_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_brace_or_switch_clause_expected);
      }
      Statement::S_switch stmt_c = { std::move(ctrl), std::move(clauses) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_do_while_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // do-while-statement ::=
      //   "do" statement "while" "(" expression ")" ";"
      if(do_match_keyword(toks_io, Token::keyword_do) == false) {
        return false;
      }
      Block body;
      if(do_accept_statement(body, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      if(do_match_keyword(toks_io, Token::keyword_while) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_keyword_while_expected);
      }
      Expression cond;
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      if(do_accept_expression(cond, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_while stmt_c = { true, std::move(cond), std::move(body) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_while_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // while-statement ::=
      //   "while" "(" expression ")" statement
      if(do_match_keyword(toks_io, Token::keyword_while) == false) {
        return false;
      }
      Expression cond;
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      if(do_accept_expression(cond, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Block body;
      if(do_accept_statement(body, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      Statement::S_while stmt_c = { false, std::move(cond), std::move(body) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_for_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // for-statement ::=
      //   "for" "(" ( for-statement-range | for-statement-triplet ) ")" statement
      // for-statement-range ::=
      //   "each" identifier "," identifier ":" expression
      // for-statement-triplet ::=
      //   ( null-statement | variable-definition | expression-statement ) expression-opt ";" expression-opt
      if(do_match_keyword(toks_io, Token::keyword_for) == false) {
        return false;
      }
      // This for-statement is ranged if and only if `key_name` is non-empty, where `step` is used as the range initializer.
      String key_name;
      String mapped_name;
      Statement init_stmt;
      Expression cond;
      Expression step;
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      if(do_match_keyword(toks_io, Token::keyword_each)) {
        if(do_accept_identifier(key_name, toks_io) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
        }
        if(do_match_punctuator(toks_io, Token::punctuator_comma) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_comma_expected);
        }
        if(do_accept_identifier(mapped_name, toks_io) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
        }
        if(do_match_punctuator(toks_io, Token::punctuator_colon) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_colon_expected);
        }
      } else {
        const bool init_got = do_accept_variable_definition(init_stmt, toks_io) ||
                              do_accept_null_statement(init_stmt, toks_io) ||
                              do_accept_expression_statement(init_stmt, toks_io);
        if(!init_got) {
          throw do_make_parser_result(toks_io, Parser_result::error_for_statement_initializer_expected);
        }
        do_accept_expression(cond, toks_io);
        if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
          throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
        }
      }
      do_accept_expression(step, toks_io);
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Block body;
      if(do_accept_statement(body, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      if(key_name.empty()) {
        Vector<Statement> init;
        init.emplace_back(std::move(init_stmt));
        Statement::S_for stmt_c = { std::move(init), std::move(cond), std::move(step), std::move(body) };
        stmt_out = std::move(stmt_c);
      } else {
        Statement::S_for_each stmt_c = { std::move(key_name), std::move(mapped_name), std::move(step), std::move(body) };
        stmt_out = std::move(stmt_c);
      }
      return true;
    }

  bool do_accept_break_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // break-statement ::=
      //   "break" ( "switch" | "while" | "for" ) ";"
      if(do_match_keyword(toks_io, Token::keyword_break) == false) {
        return false;
      }
      Statement::Target target = Statement::target_unspec;
      if(do_match_keyword(toks_io, Token::keyword_switch)) {
        target = Statement::target_switch;
        goto z;
      }
      if(do_match_keyword(toks_io, Token::keyword_while)) {
        target = Statement::target_while;
        goto z;
      }
      if(do_match_keyword(toks_io, Token::keyword_for)) {
        target = Statement::target_for;
        goto z;
      }
    z:
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_break stmt_c = { target };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_continue_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // continue-statement ::=
      //   "continue" ( "while" | "for" ) ";"
      if(do_match_keyword(toks_io, Token::keyword_continue) == false) {
        return false;
      }
      Statement::Target target = Statement::target_unspec;
      if(do_match_keyword(toks_io, Token::keyword_while)) {
        target = Statement::target_while;
        goto z;
      }
      if(do_match_keyword(toks_io, Token::keyword_for)) {
        target = Statement::target_for;
        goto z;
      }
    z:
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_continue stmt_c = { target };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_throw_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // throw-statement ::=
      //   "throw" expression ";"
      if(do_match_keyword(toks_io, Token::keyword_throw) == false) {
        return false;
      }
      Expression expr;
      if(do_accept_expression(expr, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_expression_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_throw stmt_c = { std::move(expr) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_return_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // return-statement ::=
      //   "return" expression-opt ";"
      if(do_match_keyword(toks_io, Token::keyword_return) == false) {
        return false;
      }
      Expression expr;
      do_accept_expression(expr, toks_io);
      if(do_match_punctuator(toks_io, Token::punctuator_semicol) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_semicolon_expected);
      }
      Statement::S_return stmt_c = { std::move(expr) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_try_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // try-statement ::=
      //   "try" statement "catch" "(" identifier ")" statement
      if(do_match_keyword(toks_io, Token::keyword_try) == false) {
        return false;
      }
      Block body_try;
      if(do_accept_statement(body_try, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      if(do_match_keyword(toks_io, Token::keyword_catch) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_keyword_catch_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_op) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_open_parenthesis_expected);
      }
      String except_name;
      if(do_accept_identifier(except_name, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
      }
      if(do_match_punctuator(toks_io, Token::punctuator_parenth_cl) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_close_parenthesis_expected);
      }
      Block body_catch;
      if(do_accept_statement(body_catch, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_statement_expected);
      }
      Statement::S_try stmt_c = { std::move(body_try), std::move(except_name), std::move(body_catch) };
      stmt_out = std::move(stmt_c);
      return true;
    }

  bool do_accept_nonblock_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // non-block-statement ::=
      //   null-statement |
      //   variable-definition | immutable-variable-definition | function-definition |
      //   expression-statement |
      //   if-statement | switch-statement |
      //   do-while-statement | while-statement | for-statement |
      //   break-statement | continue-statement | throw-statement | return-statement |
      //   try-statement
      return do_accept_null_statement(stmt_out, toks_io) ||
             do_accept_variable_definition(stmt_out, toks_io) ||
             do_accept_immutable_variable_definition(stmt_out, toks_io) ||
             do_accept_function_definition(stmt_out, toks_io) ||
             do_accept_expression_statement(stmt_out, toks_io) ||
             do_accept_if_statement(stmt_out, toks_io) ||
             do_accept_switch_statement(stmt_out, toks_io) ||
             do_accept_do_while_statement(stmt_out, toks_io) ||
             do_accept_while_statement(stmt_out, toks_io) ||
             do_accept_for_statement(stmt_out, toks_io) ||
             do_accept_break_statement(stmt_out, toks_io) ||
             do_accept_continue_statement(stmt_out, toks_io) ||
             do_accept_throw_statement(stmt_out, toks_io) ||
             do_accept_return_statement(stmt_out, toks_io) ||
             do_accept_try_statement(stmt_out, toks_io);
    }
  bool do_accept_nonblock_statement(Block &block_out, Token_stream &toks_io)
    {
      Statement stmt;
      if(do_accept_nonblock_statement(stmt, toks_io) == false) {
        return false;
      }
      Vector<Statement> stmts;
      stmts.emplace_back(std::move(stmt));
      block_out = std::move(stmts);
      return true;
    }

  template<typename SinkT>
    bool do_accept_statement(SinkT &sink_out, Token_stream &toks_io)
      {
        ASTERIA_DEBUG_LOG("Looking for a statement: ", toks_io.empty() ? String::shallow("<no token>") : ASTERIA_FORMAT_STRING(*(toks_io.peek_opt())));
        // statement ::=
        //   block | non-block-statement
        return do_accept_block(sink_out, toks_io) ||
               do_accept_nonblock_statement(sink_out, toks_io);
      }

  bool do_accept_directive_or_statement(Statement &stmt_out, Token_stream &toks_io)
    {
      // directive-or-statement ::=
      //   directive | statement
      // directive ::=
      //   export-directive | import-directive
      return do_accept_export_directive(stmt_out, toks_io) ||
             do_accept_import_directive(stmt_out, toks_io) ||
             do_accept_statement(stmt_out, toks_io);
    }

}

Parser_result Parser::load(Token_stream &toks_io)
  try {
    // This has to be done before anything else because of possibility of exceptions.
    this->m_stor.set(nullptr);
    ///////////////////////////////////////////////////////////////////////////
    // Phase 3
    //   Parse the document recursively.
    ///////////////////////////////////////////////////////////////////////////
    Vector<Statement> stmts;
    while(toks_io.empty() == false) {
      // document ::=
      //   directive-or-statement-list-opt
      // directive-or-statement-list-opt ::=
      //   directive-or-statement-list | ""
      // directive-or-statement-list ::=
      //   directive-or-statement directive-or-statement-list-opt
      Statement stmt;
      if(do_accept_directive_or_statement(stmt, toks_io) == false) {
        throw do_make_parser_result(toks_io, Parser_result::error_identifier_expected);
      }
      stmts.emplace_back(std::move(stmt));
    }
    // Accept the result.
    this->m_stor.emplace<Block>(std::move(stmts));
    return Parser_result(0, 0, 0, Parser_result::error_success);
  } catch(Parser_result &e) {  // Don't play this at home.
    ASTERIA_DEBUG_LOG("Parser error: ", e.get_error(), " (", Parser_result::describe_error(e.get_error()), ")");
    this->m_stor.set(e);
    return e;
  }
void Parser::clear() noexcept
  {
    this->m_stor.set(nullptr);
  }

Parser_result Parser::get_result() const
  {
    switch(this->state()) {
      case state_empty: {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
      case state_error: {
        return this->m_stor.as<Parser_result>();
      }
      case state_success: {
        return Parser_result(0, 0, 0, Parser_result::error_success);
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }
const Block & Parser::get_document() const
  {
    switch(this->state()) {
      case state_empty: {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
      case state_error: {
        ASTERIA_THROW_RUNTIME_ERROR("The previous load operation has failed.");
      }
      case state_success: {
        return this->m_stor.as<Block>();
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }
Block Parser::extract_document()
  {
    switch(this->state()) {
      case state_empty: {
        ASTERIA_THROW_RUNTIME_ERROR("No data have been loaded so far.");
      }
      case state_error: {
        ASTERIA_THROW_RUNTIME_ERROR("The previous load operation has failed.");
      }
      case state_success: {
        auto block = std::move(this->m_stor.as<Block>());
        this->m_stor.set(nullptr);
        return block;
      }
      default: {
        ASTERIA_TERMINATE("An unknown state enumeration `", this->state(), "` has been encountered.");
      }
    }
  }

}
