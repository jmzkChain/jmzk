import json

import pyjmzk
from pyjmzk import abi, ecc, libjmzk, jmzk_link

from . import base


class Action(base.BaseType):
    def __init__(self, name, domain, key, data):
        super().__init__(name=name,
                         domain=domain,
                         key=key,
                         data=data)


class NewDomainAction(Action):
    # key: name of new domain
    def __init__(self, domain, data):
        super().__init__('newdomain', domain, '.create', data)


class UpdateDomainAction(Action):
    # key: name of updating domain
    def __init__(self, domain, data):
        super().__init__('updatedomain', domain, '.update', data)


class NewGroupAction(Action):
    # key: name of new group
    def __init__(self, key, data):
        super().__init__('newgroup', '.group', key, data)


class UpdateGroupAction(Action):
    # key: name of updating group
    def __init__(self, key, data):
        super().__init__('updategroup', '.group', key, data)


class IssueTokenAction(Action):
    # domain: name of domain
    def __init__(self, domain, data):
        super().__init__('issuetoken', domain, '.issue', data)


class TransferAction(Action):
    # domain: name of domain token belongs to
    # key: name of token
    def __init__(self, domain, key, data):
        super().__init__('transfer', domain, key, data)

class DestroyTokenAcion(Action):
    # domain: name of domain token belongs to
    # key: name of token
    def __init__(self, domain, key, data):
        super().__init__('destroytoken', domain, key, data)


class AddMetaAction(Action):
    # domain: domain, group of name of domain
    # key: domain name, group name, or token name
    def __init__(self, domain, key, data):
        super().__init__('addmeta', domain, key, data)


class NewFungibleAction(Action):
    # domain: fungible
    # key: name of new fungible
    def __init__(self, key, data):
        super().__init__('newfungible', '.fungible', key, data)


class UpdFungibleAction(Action):
    def __init__(self, key, data):
        super().__init__('updfungible', '.fungible', key, data)


class IssueFungibleAction(Action):
    def __init__(self, key, data):
        super().__init__('issuefungible', '.fungible', key, data)


class TransferFtAction(Action):
    def __init__(self, key, data):
        super().__init__('transferft', '.fungible', key, data)


class jmzk2PjmzkAction(Action):
    def __init__(self, key, data):
        super().__init__('jmzk2pjmzk', '.fungible', key, data)


class NewSuspendAction(Action):
    def __init__(self, key, data):
        super().__init__('newsuspend', '.suspend', key, data)


class AprvSuspendAction(Action):
    def __init__(self, key, data):
        super().__init__('aprvsuspend', '.suspend', key, data)


class CancelSuspendAction(Action):
    def __init__(self, key, data):
        super().__init__('cancelsuspend', '.suspend', key, data)


class ExecSuspendAction(Action):
    def __init__(self, key, data):
        super().__init__('execsuspend', '.suspend', key, data)

class EveripassAction(Action):
    def __init__(self, domain, key, data):
        super().__init__('everipass', domain, key, data)

class EveripayAction(Action):
    def __init__(self, domain, key, data):
        super().__init__('everipay', domain, key, data)

class UpdateProducerAction(Action):
    def __init__(self, data):
        super().__init__('updsched', '.prodsched', '.update', data)

class ProdvoteAction(Action):
    def __init__(self, key, data):
        super().__init__('prodvote', '.prodvote', key, data)

class SetpsvbonusAction(Action):
    def __init__(self, key, data):
        super().__init__('setpsvbonus', '.bonus', key, data)

class NewlockAction(Action):
    def __init__(self, key, data):
        super().__init__('newlock', '.lock', key, data)

class ActionTypeErrorException(Exception):
    def __init__(self):
        err = 'Action_Type_Error'
        super().__init__(self, err)


def get_action_from_abi_json(action, abi_json, domain=None, key=None):
    libjmzk.init_lib()
    abi_dict = json.loads(abi_json)
    try:
        _bin = abi.json_to_bin(action, abi_json).to_hex_string()
    except:
        raise Exception('Invalid abi json', action, abi_json)

    if action == 'newdomain':
        return NewDomainAction(abi_dict['name'], _bin)
    elif action == 'updatedomain':
        return UpdateDomainAction(abi_dict['name'], _bin)
    elif action == 'newgroup':
        return NewGroupAction(abi_dict['name'], _bin)
    elif action == 'updategroup':
        return UpdateGroupAction(abi_dict['name'], _bin)
    elif action == 'issuetoken':
        return IssueTokenAction(abi_dict['domain'], _bin)
    elif action == 'transfer':
        return TransferAction(abi_dict['domain'], abi_dict['name'], _bin)
    elif action == 'destroytoken':
        return DestroyTokenAcion(abi_dict['domain'], abi_dict['name'], _bin)
    elif action == 'addmeta':
        return AddMetaAction(domain, key, _bin)
    elif action == 'newfungible':
        return NewFungibleAction(abi_dict['sym'].split('#')[1], _bin)
    elif action == 'updfungible':
        return UpdFungibleAction(abi_dict['sym_id'], _bin)
    elif action == 'issuefungible':
        return IssueFungibleAction(abi_dict['number'].split('#')[1], _bin)
    elif action == 'transferft':
        return TransferFtAction(abi_dict['number'].split('#')[1], _bin)
    elif action == 'jmzk2pjmzk':
        return jmzk2PjmzkAction(abi_dict['number'].split('#')[1], _bin)
    elif action == 'newsuspend':
        return NewSuspendAction(abi_dict['name'], _bin)
    elif action == 'aprvsuspend':
        return AprvSuspendAction(abi_dict['name'], _bin)
    elif action == 'cancelsuspend':
        return CancelSuspendAction(abi_dict['name'], _bin)
    elif action == 'execsuspend':
        return ExecSuspendAction(abi_dict['name'], _bin)
    elif action == 'everipass':
        return EveripassAction(domain, key, _bin)
    elif action == 'everipay':
        return EveripayAction(domain, key, _bin)
    elif action == 'updsched':
        return UpdateProducerAction(_bin)
    elif action == 'prodvote':
        return ProdvoteAction(abi_dict['key'], _bin)
    elif action == 'setpsvbonus':
        return SetpsvbonusAction(abi_dict['sym'].split('#')[1], _bin)
    elif action == 'newlock':
        return NewlockAction(abi_dict['name'], _bin)
    else:
        raise ActionTypeErrorException


class ActionGenerator:
    def newdomain(self, name, creator, **kwargs):
        auth_A = base.AuthorizerWeight(base.AuthorizerRef('A', creator), 1)
        auth_G = base.AuthorizerWeight(base.AuthorizerRef('G', '.OWNER'), 1)
        issue = kwargs['issue'] if 'issue' in kwargs else None
        transfer = kwargs['transfer'] if 'transfer' in kwargs else None
        manage = kwargs['manage'] if 'manage' in kwargs else None
        abi_json = base.NewDomainAbi(name=name,
                                     creator=str(creator),
                                     issue=issue or base.PermissionDef(
                                         'issue', 1, [auth_A]),
                                     transfer=transfer or base.PermissionDef(
                                         'transfer', 1, [auth_G]),
                                     manage=manage or base.PermissionDef('manage', 1, [auth_A]))
        return get_action_from_abi_json('newdomain', abi_json.dumps())

    def updatedomain(self, name, **kwargs):
        issue = kwargs['issue'] if 'issue' in kwargs else None
        transfer = kwargs['transfer'] if 'transfer' in kwargs else None
        manage = kwargs['manage'] if 'manage' in kwargs else None
        abi_json = base.UpdateDomainAbi(name=name,
                                        issue=issue,
                                        transfer=transfer,
                                        manage=manage)
        return get_action_from_abi_json('updatedomain', abi_json.dumps())

    class GroupTypeErrorException(Exception):
        def __init__(self):
            err = 'Group_Type_Error'
            super().__init__(self, err)

    def newgroup(self, name, key, group):
        abi_json = base.NewGroupAbi(name=name, group=group.dict())
        return get_action_from_abi_json('newgroup', abi_json.dumps())

    def updategroup(self, name, key, group):
        abi_json = base.UpdateGroupAbi(name=name, group=group.dict())
        return get_action_from_abi_json('updategroup', abi_json.dumps())

    def issuetoken(self, domain, names, owner):
        abi_json = base.IssueTokenAbi(
            domain, names, owner=[str(each) for each in owner])
        return get_action_from_abi_json('issuetoken', abi_json.dumps())

    def transfer(self, domain, name, to, memo):
        abi_json = base.TransferAbi(
            domain, name, to=[str(each) for each in to], memo=memo)
        return get_action_from_abi_json('transfer', abi_json.dumps())

    def destroytoken(self, domain, name):
        abi_json = base.DestroyTokenAbi(
            domain, name)
        return get_action_from_abi_json('destroytoken', abi_json.dumps())

    def newfungible(self, name, sym_name, sym, creator, total_supply, **kwargs):
        auth_A = base.AuthorizerWeight(base.AuthorizerRef('A', creator), 1)
        issue = kwargs['issue'] if 'issue' in kwargs else None
        manage = kwargs['manage'] if 'manage' in kwargs else None
        abi_json = base.NewFungibleAbi(name=name, sym_name=sym_name, sym=sym.value(),
                                       creator=str(creator),
                                       issue=issue or base.PermissionDef(
            'issue', 1, [auth_A]),
            manage=manage or base.PermissionDef(
            'manage', 1, [auth_A]),
            total_supply=total_supply)
        return get_action_from_abi_json('newfungible', abi_json.dumps())

    def updfungible(self, sym_id, **kwargs):
        issue = kwargs['issue'] if 'issue' in kwargs else None
        manage = kwargs['manage'] if 'manage' in kwargs else None
        abi_json = base.UpdFungibleAbi(sym_id=sym_id,
                                       issue=issue,
                                       manage=manage)
        return get_action_from_abi_json('updfungible', abi_json.dumps())

    def issuefungible(self, address, number, memo):
        abi_json = base.IssueFungibleAbi(
            address=str(address), number=number, memo=memo)
        return get_action_from_abi_json('issuefungible', abi_json.dumps())

    def transferft(self, _from, to, number, memo):
        abi_json = base.TransferFtAbi(
            _from=str(_from), to=str(to), number=number, memo=memo)
        return get_action_from_abi_json('transferft', abi_json.dumps())

    def jmzk2pjmzk(self, _from, to, number, memo):
        abi_json = base.jmzk2PjmzkAbi(
            _from=str(_from), to=str(to), number=number, memo=memo)
        return get_action_from_abi_json('jmzk2pjmzk', abi_json.dumps())

    def addmeta(self, meta_key, meta_value, creator, domain, key):
        abi_json = base.AddMetaAbi(meta_key, meta_value, creator.value())
        return get_action_from_abi_json('addmeta', abi_json.dumps(), domain, key)

    def newsuspend(self, name, proposer, trx):
        abi_json = base.NewSuspendAbi(name, str(proposer), trx=trx.dict())
        return get_action_from_abi_json('newsuspend', abi_json.dumps())

    def aprvsuspend(self, name, signatures):
        abi_json = base.AprvSuspendAbi(name, [str(sig) for sig in signatures])
        return get_action_from_abi_json('aprvsuspend', abi_json.dumps())

    def cancelsuspend(self, name):
        abi_json = base.CancelSuspendAbi(name)
        return get_action_from_abi_json('cancelsuspend', abi_json.dumps())

    def execsuspend(self, name, executor):
        abi_json = base.ExecSuspendAbi(name, str(executor))
        return get_action_from_abi_json('execsuspend', abi_json.dumps())

    def everipass(self, link):
        everipass = jmzk_link.jmzkLink.parse_from_jmzkli(link)
        abi_json = base.EveripassAbi(link)
        return get_action_from_abi_json('everipass', abi_json.dumps(), everipass.get_segment_str('domain'), everipass.get_segment_str('token'))

    def everipay(self, payee, number, link):
        everipay = jmzk_link.jmzkLink.parse_from_jmzkli(link)
        abi_json = base.EveripayAbi(str(payee), number, link)
        return get_action_from_abi_json('everipay', abi_json.dumps(), '.fungible', str(everipay.get_segment_int('symbol_id')))

    def updsched(self, producers):
        abi_json = base.UpdateProducerAction(producers)
        return get_action_from_abi_json('updsched', abi_json.dumps())

    def prodvote(self, producer, key, value):
        abi_json = base.ProdvoteAbi(producer, key, value)
        return get_action_from_abi_json('prodvote', abi_json.dumps())

    # def newlock(self, producer, key, value):
    #     abi_json = base.ProdvoteAbi(producer, key, value)
    #     return get_action_from_abi_json('prodvote', abi_json.dumps())

    def new_action(self, action, **args):
        func = getattr(self, action)
        return func(**args)

    def new_action_from_json(self, action, json):
        return get_action_from_abi_json(action, json)
