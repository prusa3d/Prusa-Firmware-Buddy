#!/usr/bin/env python3
"""
This script can be used to remove old Jenkins jobs for the already merged pull requests.
"""

import requests

# Go to browser and grab a cookie from Jenkins.
COOKIES = {'JSESSIONID.example': 'example'}


def apicall(url):
    response = requests.get(url + 'api/json', cookies=COOKIES)
    if response.status_code != 200:
        raise Exception(str(response))
    else:
        return response.json()


def main():
    # list all pull-request jobs
    jobs = apicall(
        'https://holly.prusa3d.com/job/Prusa-Firmware-Buddy-Private/job/Multibranch/view/change-requests/'
    )['jobs']
    for job in jobs:
        job_url = job['url']
        buildable = apicall(job_url)['buildable']
        if not buildable:
            # crumb is Jenkins term for CSRF token
            crumb = apicall('https://holly.prusa3d.com/crumbIssuer/')
            headers = {crumb['crumbRequestField']: crumb['crumb']}
            # actual request for deletion
            response = requests.post(job_url + 'doDelete',
                                     cookies=COOKIES,
                                     headers=headers)
            print(job_url, response.status_code)


if __name__ == '__main__':
    main()
