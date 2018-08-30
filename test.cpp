bool
controller::light_validation_allowed(bool replay_opts_disabled_by_policy) const {
    if(!my->pending || my->in_trx_requiring_checks) {
        return false;
    }

    auto pb_status = my->pending->_block_status;

    // in a pending irreversible or previously validated block and we have forcing all checks
    bool consider_skipping_on_replay = (pb_status == block_status::irreversible || pb_status == block_status::validated) && !replay_opts_disabled_by_policy;

    // OR in a signed block and in light validation mode
    bool consider_skipping_on_validate = (pb_status == block_status::complete && my->conf.block_validation_mode == validation_mode::LIGHT);

    return consider_skipping_on_replay || consider_skipping_on_validate;
}

bool
controller::skip_auth_check() const {
    return light_validation_allowed(my->conf.force_all_checks);
}

bool
controller::skip_db_sessions(block_status bs) const {
    bool consider_skipping = bs == block_status::irreversible;
    return consider_skipping
           && !my->conf.disable_replay_opts
           && !my->in_trx_requiring_checks;
}

bool
controller::skip_db_sessions() const {
    if(my->pending) {
        return skip_db_sessions(my->pending->_block_status);
    }
    else {
        return false;
    }
}

bool
controller::skip_trx_checks() const {
    return light_validation_allowed(my->conf.disable_replay_opts);
}
