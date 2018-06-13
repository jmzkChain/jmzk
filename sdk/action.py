import json, pyevt
from pyevt import abi

class Action:
    def __init__(self, name, domain, key, data):
        self.name = name
        self.domain = domain
        self.key = key
        self.data = data
    
    def dict(self):
        return {
            "name": self.name,
            "domain": self.domain,
            "key": self.key,
            "data": self.data
        }

class newdomainAction(Action):
    # key: name of new domain
    def __init__(self, key, data):
        super("newdomain", "domain", key, data)

class updatedomainAction(Action):
    # key: name of updating domain
    def __init__(self, key, data):
        super("updatedomain", "domain", key, data)

class newgroupAction(Action):
    # key: name of new group
    def __init__(self, key, data):
        super("newgroup", "group", key, data)

class updategroupAction(Action):
    # key: name of updating group
    def __init__(self, key, data):
        super("updategroup", "group", key, data)

class newaccountAction(Action):
    # key: name of new account
    def __init__(self, key, data):
        super("newaccount", "account", key, data)

class updateownerAction(Action):
    # key: name of updating account
    def __init__(self, key, data):
        super("updateowner", "account", key, data)

class transferevtAction(Action):
    # key: name of giving account
    def __init__(self, key, data):
        super("transferevt", "account", key, data)

class issuetokenAction(Action):
    # domain: name of domain
    def __init__(self, domain, data):
        super("issuetoken", domain, "issue", data)
    
class transferAction(Action):
    # domain: name of domain token belongs to
    # key: name of token
    def __init__(self, domain, key, data):
        super("transfer", domain, key, data)

def getActionFromAbiJson(action, abi_json):
    abi_dict = json.loads(abi_json)
    bin = abi.json_to_bin("newdomain", abi_json)
    if action == "newdomain":
        return newdomainAction(abi_dict["name"], bin)
    elif action == "updatedomain":
        return updatedomainAction(abi_dict["name"], bin)
    elif action == "newgroup":
        return newgroupAction(abi_dict["name"], bin)
    elif action == "updategroup":
        return updategroupAction(abi_dict["name"], bin)
    elif action == "newaccount":
        return newaccountAction(abi_dict["name"], bin)
    elif action == "updateowner":
        return updateownerAction(abi_dict["name"], bin)
    elif action == "transferevt":
        return transferevtAction(abi_dict["from"], bin)
    elif action == "issuetoken":
        return issuetokenAction(abi_dict["domain"], bin)
    elif action == "transfer":
        return transferAction(abi_dict["domain"], abi_dict["name"], bin)
    else:
        return None