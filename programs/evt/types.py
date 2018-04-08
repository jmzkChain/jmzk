class Action:
    def __init__(self):
        self.account = "eosio"
        self.name = ""
        self.domain = ""
        self.key = 0
        self.data = None

class Transaction:
    def __init__(self):
        self.expiration = 0
        self.region = 0
        self.ref_block_num = 0
        self.ref_block_prefix = 0
        self.packed_bandwidth_words = 0
        self.context_free_cpu_bandwidth = 0

        self.actions = []

    def sign(self, privateKey, chainId):
        pass

class Token:
    def __init__(self):
        self.domain = ""
        self.name = ""
        self.owner = []

class KeyWeight:
    def __init__(self):
        self.key = ""
        self.weight = 0

class GroupWeight:
    def __init__(self):
        self.id = 0
        self.weight = 0

class Group:
    def __init__(self):
        self.id = 0
        self.key = ""
        self.threshold = 0
        self.keys = []

class Permission:
    def __init__(self):
        self.name = ""
        self.threshold = 0
        self.groups = []

class Domain:
    def __init__(self):
        self.name = ""
        self.issuer = ""
        self.issue_time = 0
        
        self.issue = None
        self.transfer = None
        self.manage = None

class NewDomain:
    def __init__(self):
        self.name = ""
        self.issuer = ""

        self.issue = None
        self.transfer = None
        self.manage = None

        self.groups = []

class IssueToken:
    def __init__(self):
        self.domain = ""
        self.names = []
        self.owner = []

class Transfer:
    def __init__(self):
        self.domain = ""
        self.name = ""
        self.to = []

class UpdateGroup:
    def __init__(self):
        self.id = 0
        self.threshold = 0
        self.keys = []

class UpdateDomain:
    def __init__(self):
        self.name = ""
        self.issue = None
        self.transfer = None
        self.manage = None
        self.groups = []