import json

from pyevt import abi, ecc, libevt

libevt.init_lib()

# Type and Structures


class BaseType:
    def __init__(self, **kwargs):
        self.kwargs = kwargs

    def dict(self):
        return self.kwargs

    def dumps(self):
        return json.dumps(self.kwargs)


class User:
    def __init__(self):
        self.pub_key, self.priv_key = ecc.generate_new_pair()


class AuthorizerRef:
    def __init__(self, _type, key):
        self.key = key
        self.type = _type

    def value(self):
        return '[%s] %s' % (self.type, self.key)


class SymbolArgsErrorException(Exception):
    def __init__(self):
        err = 'Symobl_Args_Error'
        super().__init__(self, err)


class Symbol:
    def __init__(self, name, precision=5):
        if precision > 17 or precision < 0:
            raise SymbolArgsErrorException
        if len(name) > 6 or (not name.isupper()):
            raise SymbolArgsErrorException
        self.name = name
        self.precision = precision

    def value(self):
        return '%d,%s' % (self.precision, self.name)


def new_asset(symbol):
    def value(num):
        fmt = '%%.%df %s' % (symbol.precision, symbol.name)
        return fmt % (num)
    return value


EvtSymbol = Symbol(name='EVT', precision=5)
EvtAsset = new_asset(EvtSymbol)


class AuthorizerWeight(BaseType):
    def __init__(self, ref, weight):
        super().__init__(ref=ref.value(), weight=weight)


class PermissionDef(BaseType):
    # name: permission_name
    # threshold: uint32
    # authorizers: authorizer_weight[]
    def __init__(self, name, threshold, authorizers=[]):
        super().__init__(name=name, threshold=threshold,
                         authorizers=[auth.dict() for auth in authorizers])

    def add_authorizer(self, auth, weight):
        self.kwargs['authorizers'].append(
            AuthorizerWeight(auth, weight).dict())


# Special Type: group
class Node(BaseType):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)

    class NodeTypeException(Exception):
        def __init__(self):
            err = 'Group_Node_Type_Error'
            super().__init__(self, err)

    class NodeArgsExcetion(Exception):
        def __init__(self):
            err = 'Group_Node_Arguments_Error'
            super().__init__(self, err)

    def add_child(self, Node):
        if 'key' in self.kwargs:  # Leaf Node
            raise NodeTypeException
        if 'nodes' not in self.kwargs:  # Error in parent node
            raise NodeArgsException
        self.kwargs['nodes'].append(Node.dict())


class RootNode(Node):
    def __init__(self, threshold, nodes):
        super().__init__(threshold=threshold,
                         nodes=[node.dict() for node in nodes])


class NonLeafNode(Node):
    def __init__(self, threshold, weight, nodes):
        super().__init__(threshold=threshold,
                         weight=weight,
                         nodes=[node.dict() for node in nodes])


class LeafNode(Node):
    def __init__(self, key, weight):
        super().__init__(key=key, weight=weight)


class Group(BaseType):
    def __init__(self, name, key, root):
        super().__init__(name=name, key=key, root=root)

# Abi jsons of Actions


class NewDomainAbi(BaseType):
    def __init__(self, name, creator, issue, transfer, manage):
        super().__init__(name=name,
                         creator=creator,
                         issue=issue.dict(),
                         transfer=transfer.dict(),
                         manage=manage.dict())


class UpdateDomainAbi(BaseType):
    def __init__(self, name, issue, transfer, manage):
        super().__init__(name=name,
                         issue={} if issue == None else issue.dict(),
                         transfer={} if transfer == None else transfer.dict(),
                         manage={} if manage == None else manage.dict())


class IssueTokenAbi(BaseType):
    def __init__(self, domain, names, owner):
        super().__init__(domain=domain,
                         names=names,
                         owner=owner)


class TransferAbi(BaseType):
    def __init__(self, domain, name, to, memo):
        super().__init__(domain=domain,
                         name=name,
                         to=to,
                         memo=memo)


class NewGroupAbi(BaseType):
    def __init__(self, name, group):
        super().__init__(name=name,
                         group=group)


class UpdateGroupAbi(BaseType):
    def __init__(self, name, group):
        super().__init__(name=name,
                         group=group)


class AddMetaAbi(BaseType):
    def __init__(self, key, value, creator):
        super().__init__(key=key, value=value, creator=creator)


class NewFungibleAbi(BaseType):
    def __init__(self, sym, creator, issue, manage, total_supply):
        super().__init__(sym=sym, creator=creator, issue=issue.dict(),
                         manage=manage.dict(), total_supply=total_supply)


class UpdFungibleAbi(BaseType):
    def __init__(self, sym, issue, manage):
        super().__init__(sym=sym,
                         issue={} if issue == None else issue.dict(),
                         manage={} if manage == None else manage.dict())


class IssueFungibleAbi(BaseType):
    def __init__(self, address, number, memo):
        super().__init__(address=address, number=number, memo=memo)


class TransferFtAbi(BaseType):
    def __init__(self, _from, to, number, memo):
        args = {'from': _from, 'to': to, 'number': number, 'memo': memo}
        super().__init__(**args)
