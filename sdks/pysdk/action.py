import json

import pyevt
from pyevt import abi, ecc, libevt

import base


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
        super().__init__('newgroup', 'group', key, data)


class UpdateGroupAction(Action):
    # key: name of updating group
    def __init__(self, key, data):
        super().__init__('updategroup', 'group', key, data)


class NewAccountAction(Action):
    # key: name of new account
    def __init__(self, key, data):
        super().__init__('newaccount', 'account', key, data)


class UpdateOwnerAction(Action):
    # key: name of updating account
    def __init__(self, key, data):
        super().__init__('updateowner', 'account', key, data)


class TransferEvtAction(Action):
    # key: name of giving account
    def __init__(self, key, data):
        super().__init__('transferevt', 'account', key, data)


class IssueTokenAction(Action):
    # domain: name of domain
    def __init__(self, domain, data):
        super().__init__('issuetoken', domain, '.issue', data)


class TransferAction(Action):
    # domain: name of domain token belongs to
    # key: name of token
    def __init__(self, domain, key, data):
        super().__init__('transfer', domain, key, data)


class AddMetaAction(Action):
    # domain: domain, group of name of domain
    # key: domain name, group name, or token name
    def __init__(self, domain, key, data):
        super().__init__('addmeta', domain, key, data)


class ActionTypeErrorException(Exception):
    def __init__(self):
        err = 'Action_Type_Error'
        super().__init__(self, err)


def get_action_from_abi_json(action, abi_json, domain=None, key=None):
    libevt.init_lib()
    abi_dict = json.loads(abi_json)
    _bin = abi.json_to_bin(action, abi_json).to_hex_string()
    if action == 'newdomain':
        return NewDomainAction(abi_dict['name'], _bin)
    elif action == 'updatedomain':
        return UpdateDomainAction(abi_dict['name'], _bin)
    elif action == 'newgroup':
        return NewGroupAction(abi_dict['name'], _bin)
    elif action == 'updategroup':
        return UpdateGroupAction(abi_dict['name'], _bin)
    elif action == 'newaccount':
        return NewAccountAction(abi_dict['name'], _bin)
    elif action == 'updateowner':
        return UpdateOwnerAction(abi_dict['name'], _bin)
    elif action == 'transferevt':
        return TransferEvtAction(abi_dict['from'], _bin)
    elif action == 'issuetoken':
        return IssueTokenAction(abi_dict['domain'], _bin)
    elif action == 'transfer':
        return TransferAction(abi_dict['domain'], abi_dict['name'], _bin)
    elif action == 'addmeta':
        return AddMetaAction(domain, key, _bin)
    else:
        raise ActionTypeErrorException


class ActionGenerator:
    def newdomain(self, name, issuer, **kwargs):
        auth_A = base.AuthorizerWeight(base.AuthorizerRef('A', issuer), 1)
        auth_G = base.AuthorizerWeight(base.AuthorizerRef('G', 'OWNER'), 1)
        issue = kwargs['issue'] if 'issue' in kwargs else None
        transfer = kwargs['transfer'] if 'transfer' in kwargs else None
        manage = kwargs['manage'] if 'manage' in kwargs else None
        abi_json = base.NewDomainAbi(name=name,
                                     issuer=str(issuer),
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

    def newaccount(self, name, owner):
        abi_json = base.NewAccountAbi(
            name=name, owner=[str(each) for each in owner])
        return get_action_from_abi_json('newaccount', abi_json.dumps())

    def updateowner(self, name, owner):
        abi_json = base.UpdateOwnerAbi(
            name=name, owner=[str(each) for each in owner])
        return get_action_from_abi_json('updateowner', abi_json.dumps())

    def transferevt(self, _from, to, amount):
        abi_json = base.TransferEvtAbi(_from, to, amount)
        return get_action_from_abi_json('transferevt', abi_json.dumps())

    def issuetoken(self, domain, names, owner):
        abi_json = base.IssueTokenAbi(
            domain, names, owner=[str(each) for each in owner])
        return get_action_from_abi_json('issuetoken', abi_json.dumps())

    def transfer(self, domain, name, to):
        abi_json = base.TransferAbi(
            domain, name, to=[str(each) for each in to])
        return get_action_from_abi_json('transfer', abi_json.dumps())

    def addmeta(self, meta_key, meta_value, creator, domain, key):
        abi_json = base.AddMetaAbi(meta_key, meta_value, str(creator))
        return get_action_from_abi_json('addmeta', abi_json.dumps(), domain, key)

    def new_action(self, action, **args):
        func = getattr(self, action)
        return func(**args)

    def new_action_from_json(self, action, json):
        return get_action_from_abi_json(action, json)
