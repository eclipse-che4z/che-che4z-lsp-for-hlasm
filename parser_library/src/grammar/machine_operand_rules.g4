parser grammar machine_operand_rules; 


mach_op returns [operand_ptr op]
	: mach_expr
	{
		$op = std::make_unique<expr_machine_operand>(std::move($mach_expr.m_e),provider.get_range($mach_expr.ctx));
	}
	| disp=mach_expr lpar base=mach_expr rpar
	{
		$op = std::make_unique<address_machine_operand>(std::move($disp.m_e), nullptr, std::move($base.m_e), provider.get_range($disp.ctx->getStart(),$rpar.ctx->getStop()),checking::operand_state::ONE_OP);
	}
	| disp=mach_expr lpar index=mach_expr comma base=mach_expr rpar
	{
		$op = std::make_unique<address_machine_operand>(std::move($disp.m_e), std::move($index.m_e), std::move($base.m_e),provider.get_range($disp.ctx->getStart(),$rpar.ctx->getStop()),checking::operand_state::PRESENT);
	}
	| disp=mach_expr lpar comma base=mach_expr rpar
	{
		$op = std::make_unique<address_machine_operand>(std::move($disp.m_e), nullptr, std::move($base.m_e),provider.get_range($disp.ctx->getStart(),$rpar.ctx->getStop()),checking::operand_state::FIRST_OMITTED);
	}
	| disp=mach_expr lpar index=mach_expr comma rpar
	{
		$op = std::make_unique<address_machine_operand>(std::move($disp.m_e), std::move($index.m_e), nullptr, provider.get_range($disp.ctx->getStart(),$rpar.ctx->getStop()),checking::operand_state::SECOND_OMITTED);
	};
	