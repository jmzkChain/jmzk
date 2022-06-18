import random
import string
import time

from pyjmzksdk import base

ITEM_TYPES = ['domain', 'group', 'fungible', 'token']
REQUIREMENTS = {
    'newdomain': [],
    'updatedomain': ['domain'],
    'newgroup': [],
    'updategroup': ['group'],
    'newfungible': [],
    'updfungible': ['fungible'],
    'issuefungible': ['fungible'],
    'transferft': ['fungible'],
    'issuetoken': ['domain'],
    'transfer': ['token'],
    'addmeta': ['domain', 'group', 'token', 'fungible']
}


def fake_name(prefix):
    return prefix + ''.join(random.choice(string.ascii_letters + string.digits) for _ in range(8))


def fake_symbol(prefix, syms):
    while True:
        sym = prefix.upper() + ''.join(random.choice(string.ascii_letters[26:]) for _ in range(5))
        if sym in syms:
            continue
        syms.add(sym)
        prec = random.randint(5, 10)
        break

    return base.Symbol(name=sym, precision=prec)


class Item:
    def __init__(self, name, users):
        self.name = name
        self.users = users
        self.create_time = time.time()

    def pub_key(self):
        assert len(self.users) == 1
        return str(self.users[0].pub_key)

    def pub_keys(self):
        return [str(user.pub_key) for user in self.users]

    def priv_keys(self):
        return [user.priv_key for user in self.users]


class Domain(Item):
    def __init__(self, name, user):
        super().__init__(name, [user])


class Group(Item):
    def __init__(self, name, user):
        super().__init__(name, [user])


class Token(Item):
    def __init__(self, name, user, domain):
        super().__init__(name, [user])
        self.domain = domain


class Fungible(Item):
    def __init__(self, sym, user, total_supply):
        super().__init__(sym.name, [user])
        self.sym = sym
        self.total_supply = total_supply
        self.accounts = []


class RandomPool:
    def __init__(self, tg_name, max_user_num=2):
        self.tg_name = tg_name
        self.pool = {}
        for item in ITEM_TYPES:
            self.pool[item] = []
        self.users = [base.User() for _ in range(max_user_num)]
        self.symbols = set()

    def satisfy(self, item_type, num=1):
        return len(self.pool[item_type]) >= num

    def satisfy_action(self, action):
        requires = REQUIREMENTS[action]
        if len(requires) == 0:
            return True

        if action == 'transferft':
            for fung in self.pool['fungible']:
                if len(fung.accounts) > 0:
                    return True
            return False

        for item in requires:
            if self.satisfy(item):
                return True

        return False

    def add_item(self, item_type, item):
        self.pool[item_type].append(item)

    def get_item(self, item_type):
        return random.choice(self.pool[item_type])

    def pop_item(self, item_type):
        idx = random.randint(0, len(self.pool[item_type])-1)
        return self.pool[item_type].pop(idx)

    def get_user(self):
        return random.choice(self.users)

    def newdomain(self):
        domain = Domain(fake_name(self.tg_name), self.get_user())
        self.add_item('domain', domain)
        return {'name': domain.name, 'creator': domain.pub_key()}, domain.priv_keys()

    def updatedomain(self):
        pass

    def newgroup(self):
        pass

    def updategroup(self):
        pass

    def newfungible(self):
        sym = fake_symbol(self.tg_name, self.symbols)
        asset = base.new_asset(sym)
        fungible = Fungible(sym, self.get_user(), total_supply=100000)
        self.add_item('fungible', fungible)
        return {'sym': sym, 'creator': fungible.pub_key(), 'total_supply': asset(100000)}, fungible.priv_keys()

    def updfungible(self):
        pass
        #fungible = self.get_item['fungible']
        # return {'sym':fungible.name, 'issue':None, 'manage':None}

    def issuefungible(self):
        fungible = self.get_item('fungible')
        asset = base.new_asset(fungible.sym)
        user = self.get_user()
        fungible.accounts.append(user)
        return {'address': user.pub_key, 'number': asset(1), 'memo': fake_name('memo')}, fungible.priv_keys()

    def transferft(self):
        fungible = None
        random.shuffle(self.pool['fungible'])
        for fung in self.pool['fungible']:
            if len(fung.accounts) > 0:
                fungible = fung
                break
        asset = base.new_asset(fungible.sym)
        user1 = random.choice(fungible.accounts)
        user2 = self.get_user()  # not add user2 into accounts for convinient
        return {
            '_from': str(user1.pub_key),
            'to': str(user2.pub_key),
            'number': asset(0.0001),
            'memo': fake_name('memo')
        }, [user1.priv_key]

    def issuetoken(self):
        domain = self.get_item('domain')
        user = self.get_user()
        token = Token(fake_name('token'), user, domain)
        self.add_item('token', token)
        return {
            'domain': domain.name,
            'names': [token.name],
            'owner': token.pub_keys()
        }, domain.priv_keys()

    def transfer(self):
        token = self.pop_item('token')
        to_user = self.get_user()

        old_priv = token.priv_keys()
        token.users = [to_user]
        self.add_item('token', token)

        return {
            'domain': token.domain.name,
            'name': token.name,
            'to': [to_user.pub_key],
            'memo': fake_name('memo')
        }, old_priv

    def addmeta(self):
        item_type = None

        items = ['domain', 'token', 'group', 'fungible']
        random.shuffle(items)

        for t in items:
            if len(self.pool[t]) > 0:
                item_type = t
                break

        item = self.get_item(item_type)
        ar = base.AuthorizerRef('A', item.pub_key())

        get_domain = {
            'domain': lambda i: i.name,
            'token': lambda i: i.domain.name,
            'group': lambda _: 'group',
            'fungible': lambda _: 'fungible'
        }

        return {
            'meta_key': fake_name('meta.key'),
            'meta_value': fake_name('meta.value'),
            'creator': ar,
            'domain': get_domain[item_type](item),
            'key': item.name if item_type != 'domain' else '.meta'
        }, item.priv_keys()

    def require(self, action_type):
        return self.__getattribute__(action_type)()
