"""
Clone, and setup github repository for workers
"""

import os
import json
import sys
from github3 import login

if len(sys.argv) != 5:
    print("ERROR! Incorrect number of arguments")
    sys.exit(1)

REPO_OWNER = sys.argv[3]
REPO_NAME = sys.argv[4]

with open(sys.argv[1], "r") as credsfile:
    creds = [x.strip() for x in credsfile.readlines()]

if len(creds) != 3:
    print("ERROR: Incorrect number of lines in credentials file!")
    sys.exit(1)

sys.argv[2] = json.loads(sys.argv[2])['id']

token = creds[0]
username = creds[1]
password = creds[2]
gh = login(token=token)

if gh is None or str(gh) == "":
    print("ERROR: Could not authenticate with GitHub!")
    sys.exit(1)

pr = gh.pull_request(REPO_OWNER, REPO_NAME, int(sys.argv[2]))
if pr is None or str(pr) == "":
    print("ERROR: Could not get PR information from GitHub for PR %d" % int(sys.argv[2]))
    sys.exit(1)

branch_user = pr.head.label.split(":")[0]
branch_name = pr.head.label.split(":")[1]

repo = gh.repository(branch_user, REPO_NAME)

url = repo.clone_url
if '-dev' in REPO_NAME:
    github_start = url.find('g')
    start = url[0:github_start]
    end = url[github_start:]
    url = start + username + ":" + password + "@" + end

# clone and setup the repository for the workers
os.system("./setup-repo.sh " + url + " " + branch_name)
