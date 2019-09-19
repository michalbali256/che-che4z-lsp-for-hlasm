#include "expression_visitor.h"
#include "../processing/context_manager.h"

using namespace hlasm_plugin::parser_library::generated;
using namespace hlasm_plugin::parser_library::expressions;


antlrcpp::Any expression_visitor::visitExpr(hlasmparser::ExprContext * ctx)
{
	return antlrcpp::Any(expression::evaluate(visit(ctx->expr_p_space_c())));
}

antlrcpp::Any expression_visitor::visitExpr_test(hlasm_plugin::parser_library::generated::hlasmparser::Expr_testContext * ctx)
{
	return visit(ctx->expr_statement());
}

int temporary_counter___ = 0;

antlrcpp::Any expression_visitor::visitExpr_statement(hlasm_plugin::parser_library::generated::hlasmparser::Expr_statementContext * ctx)
{
	std::string res;
	
	if (ctx->tmp != nullptr)
	{
		++temporary_counter___;
		res = visit(ctx->tmp).as<std::string>();
		res.push_back('\n');
		
	}
	res.append(visit(ctx->expr_p()).as<expr_ptr>()->get_str_val());
	return res;
}

antlrcpp::Any expression_visitor::visitExpr_p(hlasm_plugin::parser_library::generated::hlasmparser::Expr_pContext * ctx)
{
	if (ctx->expr_sContext != nullptr)
		return visit(ctx->expr_sContext);
	else if (ctx->children.at(0)->getText() == "+")
		return visit(ctx->expr_p());
	else
		return (-*visitE(ctx->expr_p()));
}

antlrcpp::Any expression_visitor::visitExpr_s(hlasm_plugin::parser_library::generated::hlasmparser::Expr_sContext * ctx)
{
	if (ctx->t != nullptr)
		return visit(ctx->t);

	auto a = visitE(ctx->tmp);
	auto b = visitE(ctx->term_c());

	if (ctx->children.at(1)->getText() == "+")
		return (*a + *b);
	else
		return (*a - *b);
}

antlrcpp::Any expression_visitor::visitTerm_c(hlasm_plugin::parser_library::generated::hlasmparser::Term_cContext * ctx)
{
	if (ctx->t != nullptr)
		return visit(ctx->t);

	auto a = visitE(ctx->tmp);
	auto b = visitE(ctx->term());

	if (ctx->children.at(1)->getText() == "*")
		return (*a * *b);
	else
		return (*a / *b);
}

antlrcpp::Any expression_visitor::visitTerm(hlasm_plugin::parser_library::generated::hlasmparser::TermContext * ctx)
{

	if (ctx->var_symbolContext != nullptr)
	{
		auto var = processing::context_manager(hlasm_ctx_).get_var_sym_value(*ctx->var_symbolContext->vs);
		switch (var.type)
		{
		case context::SET_t_enum::A_TYPE:
			return (expr_ptr)make_arith(var.access_a());
		case context::SET_t_enum::B_TYPE:
			return (expr_ptr)make_logic(var.access_b());
		case context::SET_t_enum::C_TYPE:
			return (expr_ptr)arithmetic_expression::from_string("",var.access_c(),false);
		default:
			return (expr_ptr)make_arith(0);
		}
	}

	if (ctx->expr() != nullptr)
		return visit(ctx->expr());

	if (ctx->ca_string() != nullptr)
		return (expr_ptr)visit(ctx->ca_string()).as<char_ptr>();

	if (ctx->data_attribute() != nullptr)
		return visit(ctx->data_attribute());

	if (ctx->string() != nullptr)
		return expression::self_defining_term(
			ctx->children.at(0)->getText(),
			visit(ctx->string()).as<std::string>(),
			false); /*TODO: dbcs*/

	if (ctx->id_sub() != nullptr)
	{
		return visit(ctx->id_sub());
	}

	if (ctx->children.size() == 1)
	{
		return visit(ctx->children.at(0));
	}

	assert(ctx->exception);
	return (expr_ptr)make_arith(0);
}

antlrcpp::Any hlasm_plugin::parser_library::expressions::expression_visitor::visitId_sub(hlasm_plugin::parser_library::generated::hlasmparser::Id_subContext* ctx)
{
	auto subscript = visit(ctx->subscriptContext).as <std::vector<expr_ptr>>();
	if (subscript.empty())
		return expression::resolve_ord_symbol(*ctx->id_no_dotContext->name); /*TODO:DBCS*/
	else
	{
		assert(subscript.size() <= 2 && subscript.size() > 0);
		if (subscript.size() == 1)
			return subscript[0]->unary_operation(*ctx->id_no_dotContext->name);
		else
			return subscript[0]->binary_operation(*ctx->id_no_dotContext->name, subscript[1]);
	}
}

antlrcpp::Any expression_visitor::visitExpr_p_comma_c(hlasm_plugin::parser_library::generated::hlasmparser::Expr_p_comma_cContext * ctx)
{
	std::vector<expr_ptr> exs;
	if (ctx->exs != nullptr)
		exs = visit(ctx->exs).as< std::vector<expr_ptr>>();
	auto expr = visit(ctx->expr_pContext);
	exs.push_back(expr.as<expr_ptr>());
	return exs;
}

antlrcpp::Any expression_visitor::visitSubscript(hlasm_plugin::parser_library::generated::hlasmparser::SubscriptContext * ctx)
{
	if (ctx->children.size() >= 3)
	{
		return visit(ctx->expr_p_comma_cContext).as<std::vector<expr_ptr>>();
	}

	return std::vector<expr_ptr>();
}

antlrcpp::Any expression_visitor::visitString(hlasm_plugin::parser_library::generated::hlasmparser::StringContext * ctx)
{
	return ctx->string_ch_cContext->value;
}

antlrcpp::Any expression_visitor::visitCa_string(hlasm_plugin::parser_library::generated::hlasmparser::Ca_stringContext * ctx)
{
	if (ctx->tmp != nullptr)
		return visit(ctx->tmp).as<char_ptr>()
		->append(visit(ctx->ca_string_b()).as<char_ptr>());
	else
		return visit(ctx->ca_string_b());
}

antlrcpp::Any expression_visitor::visitCa_string_b(hlasm_plugin::parser_library::generated::hlasmparser::Ca_string_bContext * ctx)
{
	concat_chain new_chain;

	auto tmp = processing::context_manager(hlasm_ctx_).concatenate_str(ctx->string_ch_v_c()->chain);
	auto ex = make_char(tmp);
	expr_ptr s, e;
	if(ctx->substring()->e1 != nullptr)
		s = visit(ctx->substring()->e1);
	if (ctx->substring()->e2 != nullptr)
		e = visit(ctx->substring()->e2);
	return ex->substring(visit(ctx->ca_dupl_factor()).as<int>(), s, e);
}

antlrcpp::Any expression_visitor::visitCa_dupl_factor(hlasm_plugin::parser_library::generated::hlasmparser::Ca_dupl_factorContext * ctx)
{
	if (ctx->expr_p() != nullptr)
		return visit(ctx->expr_p()).as<expr_ptr>()->get_numeric_value();
	else
		return 1;
}

expression_visitor::expression_visitor(context::hlasm_context& hlasm_ctx) : hlasm_ctx_(hlasm_ctx) {}

antlrcpp::Any expression_visitor::visitExpr_p_space_c(hlasm_plugin::parser_library::generated::hlasmparser::Expr_p_space_cContext * ctx)
{
	std::deque<expr_ptr> exs;
	if (ctx->exs != nullptr)
		exs = visit(ctx->exs).as< std::deque<expr_ptr>>();
	exs.push_back(visitE(ctx->expr_p()));
	return exs;
}

antlrcpp::Any expression_visitor::visitData_attribute(hlasm_plugin::parser_library::generated::hlasmparser::Data_attributeContext * ctx)
{
	if (ctx->ORDSYMBOL()->getText() == "K" && ctx->var_symbol() && ctx->var_symbol()->vs->access_basic())
	{
		std::vector<size_t> subscript;						//TODO check expr errors
		for (auto tree : ctx->var_symbol()->vs->subscript)
			subscript.push_back((size_t)processing::context_manager(hlasm_ctx_).evaluate_expression_tree(tree)->get_set_value().to<A_t>()-1);

		auto val = hlasm_ctx_.get_data_attribute(data_attr_kind::K, hlasm_ctx_.get_var_sym(ctx->var_symbol()->vs->access_basic()->name), subscript).access_a();
		return static_cast<expr_ptr>(make_arith(val));
	}
	if (ctx->ORDSYMBOL()->getText() == "N" && ctx->var_symbol() && ctx->var_symbol()->vs->access_basic())
	{
		std::vector<size_t> subscript;						//TODO check expr errors
		for (auto tree : ctx->var_symbol()->vs->subscript)
			subscript.push_back((size_t)processing::context_manager(hlasm_ctx_).evaluate_expression_tree(tree)->get_set_value().to<A_t>() - 1);

		auto val = hlasm_ctx_.get_data_attribute(data_attr_kind::N, hlasm_ctx_.get_var_sym(ctx->var_symbol()->vs->access_basic()->name),subscript).access_a();
		return static_cast<expr_ptr>(make_arith(val));
	}
	if (ctx->ORDSYMBOL()->getText() == "D")
	{
		if (ctx->id())
		{
			auto val = hlasm_ctx_.get_data_attribute(data_attr_kind::D, ctx->id()->name).access_b();
			return static_cast<expr_ptr>(make_logic(val));
		}
		else if (ctx->var_symbol())
		{
			auto id = hlasm_ctx_.ids().add(processing::context_manager(hlasm_ctx_).get_var_sym_value(*ctx->var_symbol()->vs).to<context::C_t>());
			auto val = hlasm_ctx_.get_data_attribute(data_attr_kind::D, id).access_b();
			return static_cast<expr_ptr>(make_logic(val));
		}
	}
	if (ctx->ORDSYMBOL()->getText() == "T")
	{
		return static_cast<expr_ptr>(make_char(""));
	}

	return static_cast<expr_ptr>(make_arith(0));
}

antlrcpp::Any expression_visitor::visitNum(hlasm_plugin::parser_library::generated::hlasmparser::NumContext* ctx)
{
	return (expr_ptr)make_arith(ctx->value);
}

expr_ptr expression_visitor::visitE(antlr4::ParserRuleContext * ctx)
{
	return visit(ctx).as<expr_ptr>();
}
