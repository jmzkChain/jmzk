import json

from pyevt import abi, ecc, libevt

# Type and Structures


class BaseType:
    def __init__(self, **kwargs):
        self.kwargs = kwargs

    def dict(self):
        return self.kwargs

    def dumps(self):
        return json.dumps(self.kwargs)


class AuthorizerRef:
    def __init__(self, _type, key):
        self.key = key
        self.type = _type

    def value(self):
        return '[%s] %s' % (self.type, self.key)


class Asset:
    def __init__(self, symbol, precision=5):
        self.symbol = symbol
        self.precision = precision

    class AssetPrecisionException(Exception):
        def __init__(self):
            err = 'Asset_Precision_Exception'
            super().__init__(self, err)

    def check(self, num_str):
        if '.' in num_str:
            precision = len(num_str.split('.')[1])
        else:
            precision = 0
        if precision != self.precision:
            raise AssetPrecisionException


def new_asset(symbol, **kwargs):
    asset = Asset(symbol, **kwargs)

    def value(num_str):
        asset.check(num_str)
        return num_str + ' ' + asset.symbol
    return value


EvtAsset = new_asset('EVT', precision=5)


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
    def __init__(self, name, issuer, issue, transfer, manage):
        super().__init__(name=name,
                         issuer=issuer,
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
    def __init__(self, domain, name, to):
        super().__init__(domain=domain,
                         name=name,
                         to=to)


class NewGroupAbi(BaseType):
    def __init__(self, name, group):
        super().__init__(name=name,
                         group=group)


class UpdateGroupAbi(BaseType):
    def __init__(self, name, group):
        super().__init__(name=name,
                         group=group)


class NewAccountAbi(BaseType):
    def __init__(self, name, owner):
        super().__init__(name=name,
                         owner=owner)


class UpdateOwnerAbi(BaseType):
    def __init__(self, name, owner):
        super().__init__(name=name,
                         owner=owner)


class TransferEvtAbi(BaseType):
    def __init__(self, _from, to, amount):
        # since 'from' is a keyword of python
        args = {'from': _from, 'to': to, 'amount': amount}
        super().__init__(**args)

class AddMetaAbi(BaseType):
    def __init__(self, key, value, creator):
        super().__init__(key=key, value=value, creator=creator)

